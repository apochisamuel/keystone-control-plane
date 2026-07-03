#include "internal.hpp"

namespace aegis::detail {

IngestResult EngineCore::ingest(std::span<const Byte> data) {
    IngestResult result;
    std::vector<std::string> parse_errors;
    const auto frames = parse_capture(data, parse_errors);
    result.rejected_frames += parse_errors.size();
    result.diagnostics.insert(result.diagnostics.end(), parse_errors.begin(), parse_errors.end());
    for (const auto& frame : frames) {
        const auto raw = write_frame(frame);
        const auto source_key = "session-" + std::to_string(frame.session_id);
        std::string ingress_reason;
        if (!rate_limiter_.consume(source_key, 1.0 + static_cast<double>(raw.size()) / 256.0, ordinal_ + 1U)) {
            ++result.rejected_frames;
            result.diagnostics.push_back(describe_operation(frame.operation) + ": ingress rate limited");
            breaker_.record_failure(source_key, ordinal_, 3U, 32U);
            alerts_.raise("ingress", "warning", "rate limit for " + source_key);
            continue;
        }
        if (!breaker_.allow(source_key, ordinal_ + 1U)) {
            ++result.rejected_frames;
            result.diagnostics.push_back(describe_operation(frame.operation) + ": circuit open");
            alerts_.raise("ingress", "warning", "circuit open for " + source_key);
            continue;
        }
        if (!ingress_.allow(frame, raw.size(), ingress_reason)) {
            ++result.rejected_frames;
            result.diagnostics.push_back(describe_operation(frame.operation) + ": ingress " + ingress_reason);
            metrics_.count("frame.reject");
            health_.observe("ingress", "reject", 1, ingress_reason);
            continue;
        }
        PacketRecord record;
        record.ordinal = ++ordinal_;
        record.frame = frame;
        record.checksum = checksum32(raw);
        capture_index_.add(record);
        window_.push(record);
        archive_.append(record);
        diagnostic_pipeline_.observe(record.ordinal, frame, raw);
        capture_analyzer_.observe(record.ordinal, frame, raw.size(), record.checksum);
        statistics_.observe("frame.bytes", raw.size());
        operations_.observe_before(frame, ordinal_);
        try {
            apply(frame, result);
            ++result.accepted_frames;
            operations_.observe_after(frame, true, ordinal_);
            ingress_.observe_result(frame, true);
            breaker_.record_success(source_key);
            health_.observe("engine", "frame.accept", 1, describe_operation(frame.operation));
            if (const auto* session = sessions_.find(frame.session_id); session != nullptr) {
                timeline_.append(frame.session_id, session->generation, describe_operation(frame.operation), "accepted");
            }
        } catch (const DecodeError& error) {
            ++result.rejected_frames;
            operations_.observe_after(frame, false, ordinal_);
            result.diagnostics.push_back(describe_operation(frame.operation) + ": " + error.what());
            metrics_.count("frame.reject");
            ingress_.observe_result(frame, false);
            breaker_.record_failure(source_key, ordinal_, 3U, 32U);
            alerts_.raise("engine", "warning", error.what());
            health_.observe("engine", "frame.reject", 1, error.what());
        }
    }
    return result;
}

void EngineCore::apply(const Frame& frame, IngestResult& result) {
    (void)result;
    const auto sequence = field_u32(frame, FieldTag::Sequence, 0);
    if (sequence != 0U && !replay_.accept(frame.session_id, sequence)) throw DecodeError("replay sequence rejected");
    switch (frame.operation) {
        case Operation::Open: apply_open(frame); break;
        case Operation::Register: apply_register(frame); break;
        case Operation::StagePolicy: apply_stage(frame); break;
        case Operation::Commit: apply_commit(frame); break;
        case Operation::Data: apply_data(frame); break;
        case Operation::Handover: apply_handover(frame); break;
        case Operation::Rollback: apply_rollback(frame); break;
        case Operation::Close: apply_close(frame); break;
        case Operation::Topology: apply_topology(frame); break;
        case Operation::Audit: apply_audit(frame); break;
        case Operation::Snapshot: metrics_.count("snapshot.request"); break;
        case Operation::Recover: apply_recover(frame); break;
        case Operation::Extensions: apply_stage(frame); break;
        case Operation::Heartbeat: apply_heartbeat(frame); break;
    }
}

void EngineCore::apply_open(const Frame& frame) {
    if (frame.session_id == 0U) throw DecodeError("open frame has zero session");
    const auto profile_id = field_u32(frame, FieldTag::ProfileId, frame.session_id);
    const auto route_id = field_u32(frame, FieldTag::RouteId, profile_id);
    const auto next_hop = field_text(frame, FieldTag::NextHop, "default-hop");
    auto& session = sessions_.open(frame.session_id, profile_id, next_hop, route_id);
    session.accepted_frames++;
    Bearer bearer{route_id, 9U, session.generation, true, "initial"};
    session.bearers.install(std::move(bearer));
    if (configuration_.current() == 0U) {
        configuration_.publish({{"quota.default", "1000"}, {"scheduler.max-priority", "100"}});
    }
    quota_.configure(frame.session_id, session.quota, session.quota / 8U, session.generation);
    (void)tenants_.register_tenant(TenantProfile{frame.session_id, "tenant-" + std::to_string(frame.session_id), session.quota,
                                                  {"data", "handover", "audit"}, true});
    maintenance_.schedule("session", "quota-rollover", ordinal_ + 64U, 64U);
    const auto config_version = configuration_.current();
    events_.append(frame.session_id, "session", "open", "profile=" + std::to_string(profile_id) + ";config=" + std::to_string(config_version));
    retention_.put("session-" + std::to_string(frame.session_id), "session", session.subscriber.normalized, 256U);
    service_topology_.upsert("ingress", 4096U);
    service_topology_.upsert("policy", 4096U);
    service_topology_.upsert("dispatch", 4096U);
    service_topology_.connect("ingress", "policy");
    service_topology_.connect("policy", "dispatch");
    health_.mark("session", true, "opened");
    metrics_.count("session.open");
}

void EngineCore::apply_register(const Frame& frame) {
    auto* session = sessions_.find(frame.session_id);
    if (session == nullptr) throw DecodeError("register requires open session");
    SubscriberProfile profile;
    profile.id.value = frame.session_id;
    profile.id.normalized = field_text(frame, FieldTag::Subscriber, "subscriber-" + std::to_string(frame.session_id));
    profile.display_name = profile.id.normalized;
    profile.revision = session->generation;
    profile.permissions.insert("data");
    if (field_flag(frame, FieldTag::RouteMode)) profile.permissions.insert("route");
    identities_.put(std::move(profile));
    session->accepted_frames++;
    metrics_.count("subscriber.register");
}

void EngineCore::apply_stage(const Frame& frame) {
    auto* session = sessions_.find(frame.session_id);
    if (session == nullptr || session->closed) throw DecodeError("stage requires open session");
    PolicyCandidate candidate;
    candidate.session_id = frame.session_id;
    candidate.profile_id = field_u32(frame, FieldTag::ProfileId, session->profile == nullptr ? frame.session_id : session->profile->id + 1U);
    candidate.route_id = field_u32(frame, FieldTag::RouteId, candidate.profile_id);
    candidate.version = field_u32(frame, FieldTag::PolicyVersion, static_cast<std::uint32_t>(session->generation + 1U));
    candidate.next_hop = field_text(frame, FieldTag::NextHop, "staged-hop");
    candidate.label = field_text(frame, FieldTag::Label, candidate.next_hop);
    candidate.extension = field_bytes(frame, FieldTag::Extension);
    candidate.mirror_audit = field_flag(frame, FieldTag::MirrorAudit);
    candidate.route_mode = field_flag(frame, FieldTag::RouteMode);
    const auto policy_text = "profile=" + std::to_string(candidate.profile_id) + "\nroute=" +
                             std::to_string(candidate.route_id) + "\nversion=" +
                             std::to_string(candidate.version) + "\nnext-hop=" + candidate.next_hop +
                             "\nlabel=" + candidate.label;
    const auto policy_document = policy_text_.parse(policy_text);
    metrics_.observe("policy.text.warnings", policy_text_.validate(policy_document).size());
    sessions_.stage_profile(frame.session_id, candidate.profile_id, candidate.next_hop, candidate.route_id);
    const auto staged_label = candidate.label;
    const auto staged_generation = session->generation + 1U;
    policies_.stage(std::move(candidate));
    const auto transaction = transactions_.begin(frame.session_id, staged_generation, "policy-stage");
    transactions_.add_step(transaction, "profile", "stage");
    transactions_.add_step(transaction, "routing", "prepare");
    transactions_.apply(transaction, "profile", "staged");
    transactions_.apply(transaction, "routing", "prepared");
    transactions_.commit(transaction);
    events_.append(frame.session_id, "policy", "stage", "label=" + staged_label);
    health_.mark("policy", true, "staged");
    session->accepted_frames++;
    metrics_.count("policy.stage");
}

void EngineCore::apply_commit(const Frame& frame) {
    auto* session = sessions_.find(frame.session_id);
    if (session == nullptr) throw DecodeError("commit requires open session");
    if (!policies_.has_stage(frame.session_id)) throw DecodeError("commit lacks staged policy");
    sessions_.commit_profile(frame.session_id);
    policies_.commit(frame.session_id);
    quota_.configure(frame.session_id, session->quota, session->quota / 8U, session->generation);
    const auto committed_state = serialize_session(*session);
    checkpoints_.capture(frame.session_id, session->generation, "policy-commit", committed_state);
    versioned_snapshots_.capture(frame.session_id, session->generation, "policy-commit", committed_state);
    events_.append(frame.session_id, "policy", "commit", "generation=" + std::to_string(session->generation));
    retention_.put("policy-" + std::to_string(frame.session_id), "policy", std::to_string(session->generation), 512U);
    health_.mark("policy", true, "committed");
    session->accepted_frames++;
    metrics_.count("policy.commit");
}

void EngineCore::apply_data(const Frame& frame) {
    auto* session = sessions_.find(frame.session_id);
    if (session == nullptr || session->closed) throw DecodeError("data requires open session");
    const auto* policy = policies_.active(frame.session_id);
    if (!tenants_.validate(frame.session_id, "data").empty()) throw DecodeError("tenant capability rejected");
    const auto payload = field_bytes(frame, FieldTag::Payload);
    const auto rule = rules_.evaluate_policy(policy, *session, std::string_view(reinterpret_cast<const char*>(payload.data()), payload.size()));
    if (!rule.matched && policy != nullptr && !policy->rules.empty()) throw DecodeError("policy rule did not match");
    const auto requested = std::max<std::uint64_t>(1U, payload.size() + 1U);
    const auto reservation = quota_.reserve(frame.session_id, requested, session->generation);
    if (!reservation.has_value()) throw DecodeError("quota reservation rejected");
    const auto planned = route_planner_.select(*session, policy, policy == nullptr ? std::string_view{} : std::string_view(policy->route_label));
    if (!planned.has_value()) throw DecodeError("route planner found no route");
    const auto decision = dispatcher_.dispatch(*session, policy);
    if (!quota_.settle(reservation->token, requested)) throw DecodeError("quota settlement failed");
    const auto reference = "data-" + std::to_string(frame.session_id) + "-" + std::to_string(session->accepted_frames);
    charging_.post(frame.session_id, session->generation, requested, "data", reference);
    scheduler_.enqueue(frame.session_id, static_cast<std::uint32_t>(session->generation % 10U + 1U), ordinal_, session->generation, "dispatch", decision);
    usage_.record(frame.session_id, session->generation, payload.size(), decision, true);
    events_.append(frame.session_id, "dispatch", "data", rules_.explain(rule) + ";" + route_planner_.explain(*planned));
    replicas_.write(frame.session_id, session->generation, "dispatch", decision);
    statistics_.observe("dispatch.length", decision.size());
    metrics_.count("data.accept");
    metrics_.observe("dispatch.length", decision.size());
    session->accepted_frames++;
}

void EngineCore::apply_handover(const Frame& frame) {
    auto* session = sessions_.find(frame.session_id);
    if (session == nullptr || session->closed) throw DecodeError("handover requires open session");
    if (!tenants_.validate(frame.session_id, "handover").empty()) throw DecodeError("tenant handover capability rejected");
    const auto destination = field_u32(frame, FieldTag::HandoverTarget, frame.session_id + 1U);
    Tunnel tunnel{frame.session_id, destination, session->generation, true, "peer-" + std::to_string(destination)};
    session->tunnels.bind(std::move(tunnel));
    recovery_.capture_handover(*session, destination);
    const auto transfer = mobility_.begin(frame.session_id, frame.session_id, destination, session->generation);
    mobility_.admit(transfer, "target-admitted");
    mobility_.transfer(transfer, "bearers-transferred");
    mobility_.complete(transfer, "route-confirmed");
    charging_.post(frame.session_id, session->generation, 1U, "handover", "handover-" + std::to_string(transfer));
    events_.append(frame.session_id, "mobility", "handover", "target=" + std::to_string(destination));
    session->accepted_frames++;
    metrics_.count("handover.begin");
}

void EngineCore::apply_rollback(const Frame& frame) {
    auto* session = sessions_.find(frame.session_id);
    if (session == nullptr) throw DecodeError("rollback requires session");
    policies_.rollback(frame.session_id);
    session->staged_profile.reset();
    events_.append(frame.session_id, "policy", "rollback", "generation=" + std::to_string(session->generation));
    health_.mark("policy", true, "rolled-back");
    session->accepted_frames++;
    metrics_.count("policy.rollback");
}

void EngineCore::apply_close(const Frame& frame) {
    if (auto* session = sessions_.find(frame.session_id); session != nullptr) {
        mobility_.close_generation(frame.session_id, session->generation);
        checkpoints_.capture(frame.session_id, session->generation, "close", serialize_session(*session));
    }
    sessions_.close(frame.session_id);
    dispatcher_.invalidate(frame.session_id);
    scheduler_.cancel_session(frame.session_id);
    replay_.reset(frame.session_id);
    events_.append(frame.session_id, "session", "close", "closed");
    metrics_.count("session.close");
}

void EngineCore::apply_topology(const Frame& frame) {
    auto* session = sessions_.find(frame.session_id);
    if (session == nullptr) throw DecodeError("topology requires session");
    auto data = field_bytes(frame, FieldTag::Topology);
    if (data.empty()) data = field_bytes(frame, FieldTag::Payload);
    topologies_[frame.session_id] = decode_topology(data);
    events_.append(frame.session_id, "topology", "store", "bytes=" + std::to_string(data.size()));
    session->accepted_frames++;
    metrics_.count("topology.store");
}

void EngineCore::apply_audit(const Frame& frame) {
    auto* session = sessions_.find(frame.session_id);
    if (session == nullptr) throw DecodeError("audit requires session");
    const auto topology = topologies_.find(frame.session_id);
    if (topology != topologies_.end()) {
        TopologyInspector inspector;
        metrics_.observe("topology.summary", inspector.summarize(topology->second).size());
    }
    const auto* policy = policies_.active(frame.session_id);
    const auto checks = integrity_.session(*session);
    const auto policy_checks = integrity_.policy(policy);
    const auto integrity_text = integrity_.summarize(checks) + integrity_.summarize(policy_checks);
    if (integrity_text.find("=fail") != std::string::npos) alerts_.raise("integrity", "warning", integrity_text);
    const auto rendered = diagnostics_.render_session(*session, policy);
    audit_pipeline_.submit(frame.session_id, "engine", "audit", integrity_text, true);
    snapshot_catalog_.record(frame.session_id, session->generation, "audit", Bytes(rendered.begin(), rendered.end()));
    policies_.journal().record(frame.session_id, "audit render", stable_digest(rendered));
    checkpoints_.capture(frame.session_id, session->generation, "audit", rendered);
    events_.append(frame.session_id, "diagnostics", "audit", std::to_string(rendered.size()));
    health_.mark("diagnostics", true, "audit-rendered");
    metrics_.observe("audit.length", rendered.size());
    session->accepted_frames++;
}

void EngineCore::apply_recover(const Frame& frame) {
    const auto count = field_u32(frame, FieldTag::RecoveryCount, 1U);
    recovery_.restore_synthetic(count, frame.session_id);
    const auto applied = recovery_.apply_tickets();
    const auto actions = recovery_orchestrator_.plan(recovery_.entries(), checkpoints_.all());
    const auto recovery_errors = recovery_orchestrator_.validate(actions);
    metrics_.observe("recovery.actions", actions.size());
    metrics_.observe("recovery.plan.errors", recovery_errors.size());
    charging_.post(frame.session_id, 0U, applied, "recovery", "recovery-" + std::to_string(ordinal_));
    events_.append(frame.session_id, "recovery", "apply", "count=" + std::to_string(applied));
    metrics_.count("recovery.apply", applied);
}

void EngineCore::apply_heartbeat(const Frame& frame) {
    metrics_.count("heartbeat");
    metrics_.observe("window.size", window_.size());
    const auto ready = scheduler_.take_ready(ordinal_, 8U);
    for (const auto& item : ready) events_.append(item.session_id, "scheduler", "run", item.kind);
    const auto maintenance = maintenance_.due(ordinal_, 8U);
    for (const auto& task : maintenance) events_.append(frame.session_id, task.component, task.action, "maintenance");
    quota_.expire(ordinal_, 128U);
    retention_.expire(1U);
    reassembler_.discard_before(ordinal_ > 256U ? ordinal_ - 256U : 0U);
    if (auto* session = sessions_.find(frame.session_id); session != nullptr) session->accepted_frames++;
}

EngineStats EngineCore::stats() const {
    EngineStats result;
    result.sessions = sessions_.size();
    result.policies = policies_.active_count();
    for (const auto* session : sessions_.all()) {
        if (session != nullptr && session->profile != nullptr) result.routes += session->profile->routes.size();
    }
    result.audits = policies_.journal().size();
    for (const auto& [id, graph] : topologies_) { (void)id; result.topology_nodes += graph.nodes.size(); }
    result.recovery_entries = recovery_.size();
    result.replay_rejections = replay_.rejected();
    return result;
}

std::string EngineCore::inspect() const {
    ReportAssembler report;
    report.section("engine");
    report.field("sessions", static_cast<std::uint64_t>(sessions_.size()));
    report.field("policies", static_cast<std::uint64_t>(policies_.active_count()));
    report.field("events", static_cast<std::uint64_t>(events_.tail(1000000U).size()));
    report.section("services");
    report.field("quota", quota_.report());
    report.field("charging", charging_.receipt(0));
    report.field("scheduler", scheduler_.report());
    report.field("mobility", mobility_.report());
    report.field("health", health_.report());
    report.field("ingress", ingress_.report());
    report.field("usage", usage_.report());
    report.field("rate", rate_limiter_.report());
    report.field("breaker", breaker_.report());
    report.field("capture", capture_analyzer_.report());
    report.field("statistics", statistics_.report());
    report.field("maintenance", maintenance_.report());
    report.field("audit-pipeline", std::to_string(audit_pipeline_.size()));
    report.field("alerts", alerts_.report());
    report.field("archive", archive_.report());
    report.field("diagnostic-pipeline", diagnostic_pipeline_.render(12U));
    report.field("versioned-snapshots", versioned_snapshots_.report());
    report.field("service-topology", service_topology_.report());
    return diagnostics_.render_engine(sessions_.all(), policies_.journal()) + metrics_.report() + report.text();
}

Bytes EngineCore::snapshot() const {
    Writer writer;
    const auto recovery_data = recovery_.snapshot();
    writer.u32(static_cast<std::uint32_t>(recovery_data.size()));
    writer.bytes(recovery_data);
    const auto sessions = sessions_.all();
    writer.u16(static_cast<std::uint16_t>(std::min<std::size_t>(sessions.size(), 65535U)));
    for (const auto* session : sessions) {
        if (session == nullptr) continue;
        const auto text = serialize_session(*session);
        if (text.size() > 65535U) continue;
        writer.u16(static_cast<std::uint16_t>(text.size()));
        writer.text(text);
    }
    return writer.take();
}

IngestResult EngineCore::restore(std::span<const Byte> data) {
    IngestResult result;
    try {
        Cursor cursor(data);
        const auto recovery_size = cursor.u32();
        recovery_.restore_snapshot(cursor.bytes(recovery_size));
        const auto sessions = cursor.u16();
        for (std::uint16_t index = 0; index < sessions; ++index) {
            const auto restored = deserialize_session(cursor.text(cursor.u16()));
            if (!restored.has_value()) { ++result.rejected_frames; continue; }
            const auto& source = *restored;
            auto& target = sessions_.open(source.id, source.profile == nullptr ? source.id : source.profile->id,
                                          source.profile == nullptr || source.profile->routes.empty() ? "restored" : source.profile->routes.front().next_hop,
                                          source.profile == nullptr || source.profile->routes.empty() ? source.id : source.profile->routes.front().id);
            target.quota = source.quota;
            ++result.accepted_frames;
        }
        if (!cursor.empty()) throw DecodeError("trailing restore bytes");
    } catch (const std::exception& error) {
        ++result.rejected_frames;
        result.diagnostics.push_back(error.what());
    }
    return result;
}

void EngineCore::reset() {
    sessions_.clear();
    policies_.clear();
    dispatcher_.clear();
    identities_ = IdentityMap{};
    replay_ = ReplayGuard{};
    recovery_.clear();
    capture_index_.clear();
    window_.clear();
    metrics_.clear();
    quota_.clear();
    charging_.clear();
    scheduler_.clear();
    mobility_.clear();
    events_.clear();
    checkpoints_.clear();
    health_.clear();
    reassembler_.clear();
    retention_.clear();
    transactions_.clear();
    configuration_.clear();
    usage_.clear();
    ingress_.clear();
    rate_limiter_.clear();
    breaker_.clear();
    tenants_.clear();
    maintenance_.clear();
    statistics_.clear();
    timeline_.clear();
    snapshot_catalog_.clear();
    audit_pipeline_.clear();
    replicas_.clear();
    capture_analyzer_.clear();
    alerts_.clear();
    archive_.clear();
    diagnostic_pipeline_.clear();
    versioned_snapshots_.clear();
    service_topology_.clear();
    operations_.clear();
    topologies_.clear();
    ordinal_ = 0;
}

}  // namespace aegis::detail
