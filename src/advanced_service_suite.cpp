#include "internal.hpp"

namespace aegis::detail {

AdvancedServiceSuite::AdvancedServiceSuite() {
    balancer_.upsert("primary", 8U, true);
    balancer_.upsert("secondary", 3U, true);
    overlay_.set("base", {{"mode", "steady"}, {"version", "1"}});
    qos_.put({1U, 9U, 4096U, 25U});
    planner_.register_service("policy", {});
    planner_.register_service("routing", {"policy"});
    planner_.register_service("dispatch", {"routing"});
    resources_.connect("policy", "routing");
    resources_.connect("routing", "dispatch");
}

void AdvancedServiceSuite::observe(const Frame& frame, bool accepted, std::uint64_t tick) {
    const auto session_id = frame.session_id;
    const auto operation = describe_operation(frame.operation);
    const auto payload = field_bytes(frame, FieldTag::Payload);
    const auto fingerprint = stable_digest(operation + ":" + std::to_string(session_id) + ":" + std::to_string(frame.fields.size()));
    fingerprints_.add(session_id, fingerprint);
    metrics_.observe("frames", 1U);
    metrics_.observe("fields", frame.fields.size());
    latency_.observe(operation, tick % 100U + frame.fields.size());
    rates_.observe(operation, tick, payload.size());
    const auto record = PacketRecord{tick, frame, checksum32(write_frame(frame))};
    replay_.append(record);
    archive_.append(session_id, tick, tick, accepted ? "accepted:" + operation : "rejected:" + operation);
    subscribers_.upsert(session_id, field_text(frame, FieldTag::Subscriber, "subscriber-" + std::to_string(session_id)), "default");
    accounting_.post(session_id, payload.size(), tick);
    if (!accepted) {
        errors_.record("frame-rejected", session_id);
        retries_.schedule(operation + ":" + std::to_string(session_id), tick + 2U);
        congestion_.adjust("default", std::max<std::uint64_t>(1U, payload.size()), 0U, 256U);
        return;
    }
    const auto peer = peers_.select("default");
    if (!peer) peers_.upsert({session_id == 0U ? 1U : session_id, "default", 1024U, 0U, true});
    const auto destination = balancer_.choose(tick);
    const auto delivered = destination ? payload.size() : 0U;
    const auto adjustment = congestion_.adjust("default", std::max<std::uint64_t>(1U, payload.size()), delivered, 256U);
    metrics_.observe("allowance", adjustment.allowance);
    events_.append({tick, tick, session_id, "advanced", operation, destination.value_or("none"), fingerprint});
    audits_.retain({session_id, tick, fingerprint, tick});
    if (frame.operation == Operation::Open) {
        lifecycle_.note(session_id, 0U, "opened", tick);
        tunnels_.bind(session_id, {session_id, 1U});
        auto routes = std::vector<Route>{{field_u32(frame, FieldTag::RouteId, session_id), field_text(frame, FieldTag::NextHop, "default"), 1U, 0U, {"default"}, true}};
        auto [resolved, conflicts] = conflicts_.resolve(std::move(routes));
        (void)conflicts_.render(conflicts);
        (void)resolved;
    }
    if (frame.operation == Operation::StagePolicy) {
        lifecycle_.note(session_id, field_u32(frame, FieldTag::PolicyVersion, tick), "staged", tick);
        const auto rules = rules_.normalize({{field_u32(frame, FieldTag::RouteId, session_id), field_u32(frame, FieldTag::Quota, 1U), field_text(frame, FieldTag::Label, ""), tick}});
        (void)rules_.validate(rules);
    }
    if (frame.operation == Operation::Commit) {
        lifecycle_.note(session_id, field_u32(frame, FieldTag::PolicyVersion, tick), "committed", tick);
        const std::map<std::string, std::string> desired{{"state", "committed"}, {"route", field_text(frame, FieldTag::NextHop, "default")}};
        const std::map<std::string, std::string> observed{{"state", "staged"}};
        (void)reconciler_.apply(observed, reconciler_.reconcile(desired, observed));
    }
    if (frame.operation == Operation::Data) {
        const auto entry = accounting_.post(session_id, payload.size(), tick);
        accounting_.settle(entry);
        (void)qos_.get(1U);
        (void)tunnels_.resolve(session_id);
        (void)retries_.due(tick, 4U);
    }
    if (frame.operation == Operation::Handover) {
        tunnels_.bind(session_id, {session_id, field_u32(frame, FieldTag::HandoverTarget, 1U)});
        lifecycle_.note(session_id, tick, "handover", tick);
    }
    if (frame.operation == Operation::Topology) {
        TopologyGraph graph;
        graph.nodes.push_back({0U, {}, "root"});
        (void)graphs_.hubs(graph, 1U);
        (void)graphs_.leaves(graph);
        (void)graphs_.render(graph);
    }
    if (frame.operation == Operation::Audit) {
        (void)audits_.query(session_id);
        (void)events_.between(tick > 16U ? tick - 16U : 0U, tick);
    }
    archive_.prune(session_id, 16U);
    audits_.trim(tick > 1024U ? tick - 1024U : 0U);
}

std::string AdvancedServiceSuite::report() const {
    std::vector<std::vector<std::string>> rows{{"metric", "value"}};
    rows.push_back({"frame_total", std::to_string(metrics_.total("frames"))});
    rows.push_back({"frame_average_fields", std::to_string(metrics_.average("fields").value_or(0U))});
    rows.push_back({"operation_rate", std::to_string(rates_.rate("data", 0U))});
    rows.push_back({"planner_errors", std::to_string(planner_.validate().size())});
    rows.push_back({"top_errors", std::to_string(errors_.top(10U).size())});
    rows.push_back({"service_plan", join_labels(planner_.startup_plan(), ",")});
    rows.push_back({"overlay_mode", overlay_.resolve("mode").value_or("unknown")});
    rows.push_back({"latency_p95", std::to_string(latency_.percentile("data", 95U).value_or(0U))});
    return render_table(rows);
}

void AdvancedServiceSuite::clear() {
    balancer_.clear(); latency_.clear(); archive_.clear(); replay_.clear(); peers_.clear(); events_.clear(); retries_.clear();
    overlay_.clear(); rates_.clear(); audits_.clear(); lifecycle_.clear(); congestion_.clear(); qos_.clear(); tunnels_.clear();
    subscribers_.clear(); accounting_.clear(); errors_.clear(); fingerprints_.clear(); metrics_.clear(); resources_.clear(); planner_.clear();
}

}  // namespace aegis::detail
