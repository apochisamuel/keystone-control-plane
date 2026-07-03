#include "internal.hpp"

namespace aegis::detail {

ResilienceServiceSuite::ResilienceServiceSuite() {
    shedder_.configure(8192U, 50U);
    budgets_.configure("default", 1U << 20U);
    extensions_.register_schema({1U, 0U, 4096U, "payload"});
    extensions_.register_schema({2U, 4U, 128U, "routing"});
    keys_.rotate("control", "initial", 0U);
    quotas_.assign("dispatcher", 4096U);
    traffic_.set("default", "default", true);
}

void ResilienceServiceSuite::observe(const Frame& frame, bool accepted, std::uint64_t tick) {
    const auto session_id = frame.session_id;
    const auto operation = describe_operation(frame.operation);
    scenarios_.observe(frame, accepted);
    const auto payload = field_bytes(frame, FieldTag::Payload);
    recent_.push_back(frame);
    if (recent_.size() > 32U) recent_.erase(recent_.begin());
    const auto load = coalescer_.count_payload(recent_);
    const auto priority = operation == "commit" || operation == "recover" ? 100U : 10U;
    const auto decision = shedder_.decide(load, priority);
    patterns_.observe(session_id, frame.operation);
    groups_.put(session_id, frame.operation == Operation::Open ? "new" : "active");
    schedule_.observe(operation, tick);
    pipeline_.enqueue(operation);
    replication_.observe("local", tick, accepted ? tick : tick > 0U ? tick - 1U : 0U);
    const auto extension = field_bytes(frame, FieldTag::Extension);
    if (!extension.empty() && !extensions_.accepts(extension.front(), extension.size())) {
        incidents_.append(tick, "warning", "extension outside registered schema");
    }
    if (!accepted) {
        incidents_.append(tick, "error", operation + " rejected");
        traffic_.set("default", "default", false);
        return;
    }
    traffic_.set("default", "default", true);
    const auto granted = quotas_.grant("dispatcher", std::max<std::uint64_t>(1U, payload.size()));
    if (!decision.allowed || granted == 0U || !budgets_.consume("default", std::min<std::uint64_t>(granted, budgets_.remaining("default")))) {
        incidents_.append(tick, "warning", "observer budget pressure");
    }
    if (frame.operation == Operation::Open) {
        keys_.rotate("session-" + std::to_string(session_id), stable_digest(std::to_string(session_id)), tick);
        failover_.install(session_id, {{field_u32(frame, FieldTag::RouteId, session_id), field_text(frame, FieldTag::NextHop, "default"), 10U, 0U, {"default"}, true}});
    }
    if (frame.operation == Operation::StagePolicy) {
        PolicyCandidate candidate;
        candidate.session_id = session_id;
        candidate.route_id = field_u32(frame, FieldTag::RouteId, session_id);
        candidate.version = field_u32(frame, FieldTag::PolicyVersion, tick);
        candidate.next_hop = field_text(frame, FieldTag::NextHop, "default");
        conflicts_.note(candidate);
    }
    if (frame.operation == Operation::Data) {
        const auto route = failover_.select(session_id, {});
        if (!route) incidents_.append(tick, "warning", "no observer route");
        const auto peer = field_u32(frame, FieldTag::HandoverTarget, 1U);
        replication_.observe("peer-" + std::to_string(peer), tick + payload.size(), tick);
    }
    if (frame.operation == Operation::Audit) {
        const JournalEntry entry{tick, tick, session_id, "resilience", operation, "accepted", signer_.sign(operation, "control")};
        const auto compacted = compactor_.compact({entry}, 8U);
        if (compactor_.digest(compacted).empty()) incidents_.append(tick, "warning", "empty compaction digest");
    }
    if (frame.operation == Operation::Snapshot) {
        Checkpoint point;
        point.id = tick;
        point.session_id = session_id;
        point.generation = tick;
        point.reason = "observer";
        point.state = operation;
        point.digest = stable_digest(point.reason + ":" + point.state + ":" + std::to_string(point.generation));
        if (!snapshots_.verify(point)) incidents_.append(tick, "error", "observer snapshot mismatch");
    }
    if (frame.operation == Operation::Topology) {
        auto coalesced = coalescer_.coalesce(recent_);
        (void)coalesced;
    }
    (void)groups_.find("active");
    (void)patterns_.summary(session_id);
    (void)schedule_.next(operation);
    (void)replication_.behind(32U);
    (void)failover_.select(session_id, {});
    (void)keys_.active("control");
    (void)traffic_.allowed("default", "default");
}

std::string ResilienceServiceSuite::report() const {
    std::vector<std::vector<std::string>> rows{{"metric", "value"}};
    rows.push_back({"recent_frames", std::to_string(recent_.size())});
    rows.push_back({"default_budget", std::to_string(budgets_.remaining("default"))});
    rows.push_back({"pipeline_pending", std::to_string(pipeline_.pending().size())});
    rows.push_back({"policy_conflicts", std::to_string(conflicts_.conflicts().size())});
    rows.push_back({"replicas_behind", std::to_string(replication_.behind(32U).size())});
    rows.push_back({"incidents", std::to_string(incidents_.since(0U).size())});
    rows.push_back({"service_quota", quotas_.report()});
    rows.push_back({"normalized", telemetry_.render(telemetry_.normalize({{"recent", static_cast<std::uint64_t>(recent_.size())}}))});
    rows.push_back({"scenario_findings", std::to_string(scenarios_.findings().size())});
    return render_table(rows);
}

void ResilienceServiceSuite::clear() {
    shedder_.clear(); replication_.clear(); conflicts_.clear(); groups_.clear(); budgets_.clear(); patterns_.clear(); failover_.clear();
    extensions_.clear(); pipeline_.clear(); keys_.clear(); quotas_.clear(); schedule_.clear(); incidents_.clear(); traffic_.clear(); scenarios_.clear(); recent_.clear();
}

}  // namespace aegis::detail
