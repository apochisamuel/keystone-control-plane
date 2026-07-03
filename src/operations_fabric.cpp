#include "internal.hpp"

namespace aegis::detail {

OperationsFabric::OperationsFabric() {
    admission_.configure(0U, 4096U);
    capacity_.set_capacity("uplink", 1U << 30U);
    capacity_.set_capacity("control", 1U << 20U);
    endpoints_.upsert("default", "fabric://default", 1U);
    domains_.mark("default", true);
    catalog_.register_service("dispatcher", "1", {"route", "data"});
    catalog_.register_service("recovery", "1", {"restore", "snapshot"});
    tariff_.set_rate("data", 1U);
    classifier_.set_weight("data", 10U);
    classifier_.set_weight("commit", 80U);
    audit_.allow("engine");
    audit_.allow("policy");
    errors_.configure("engine", 128U);
    versions_.register_version("agf", 1U, 1U);
    dependencies_.add("dispatcher", "routing");
    dependencies_.add("routing", "policy");
}

void OperationsFabric::observe_before(const Frame& frame, std::uint64_t tick) {
    const auto session_id = frame.session_id;
    const auto op = describe_operation(frame.operation);
    const auto payload = field_bytes(frame, FieldTag::Payload);
    messages_.record(frame.operation);
    advanced_.observe(frame, true, tick);
    resilience_.observe(frame, true, tick);
    commands_.accept(session_id, op + ":" + std::to_string(field_u32(frame, FieldTag::Sequence, 0U)), tick);
    sequences_.observe(session_id, field_u32(frame, FieldTag::Sequence, static_cast<std::uint32_t>(tick)));
    const auto category = workloads_.classify(payload);
    workloads_.observe(category);
    telemetry_.sample("frame.bytes", payload.size());
    telemetry_.sample("frame.fields", frame.fields.size());
    queue_.push(tick + 1U, session_id, op);
    timers_.schedule(tick + 8U, "expire:" + std::to_string(session_id));
    retention_.remember("frame:" + std::to_string(session_id) + ":" + std::to_string(tick), tick);
    audit_.record("engine");
    const auto tx = transactions_.begin(op + ":" + std::to_string(session_id));
    if (tx > 1U) transactions_.link(tx, tx - 1U);
    transactions_.complete(tx);
    if (frame.operation == Operation::Open) {
        acl_.grant(session_id, "data");
        acl_.grant(session_id, "audit");
        admission_.admit(session_id, 0U);
        connections_.open(session_id, field_text(frame, FieldTag::NextHop, "default"), tick);
        correlations_.link(field_text(frame, FieldTag::Subscriber, "anonymous"), session_id);
        slices_.attach(session_id, "default");
        subscriptions_.subscribe(session_id, "health");
        zones_.assign(session_id, "default");
        domains_.assign(session_id, "default");
        budgets_.configure(session_id, field_u32(frame, FieldTag::Quota, 4096U));
        replicas_.plan(session_id, {"replica-a", "replica-b"});
        leases_.acquire("session:" + std::to_string(session_id), session_id, tick + 128U);
        identities_.observe(session_id, field_text(frame, FieldTag::Subscriber, "anonymous"), field_text(frame, FieldTag::NextHop, "default"));
        peers_.register_peer(session_id, field_text(frame, FieldTag::NextHop, "default"), "default");
        topology_.put(session_id, {});
        snapshots_.capture(session_id, 0U, {});
    }
    if (frame.operation == Operation::StagePolicy || frame.operation == Operation::Commit) {
        schedule_.schedule(session_id, tick + 2U, op);
        constraints_.set(session_id, {});
        interpreter_.load(session_id, {});
    }
    if (frame.operation == Operation::Handover) {
        const auto target = field_u32(frame, FieldTag::HandoverTarget, 1U);
        const auto plan = handovers_.propose(session_id, 0U, target);
        handovers_.approve(plan);
        replicas_.promote(session_id);
    }
    if (frame.operation == Operation::Topology) {
        topology_.put(session_id, {});
    }
    if (frame.operation == Operation::Data) {
        const auto units = std::max<std::uint64_t>(1U, payload.size());
        capacity_.reserve("uplink", units);
        uplink_.add(session_id, units);
        flows_.add(session_id, category, units);
        transit_.offer(session_id, units);
        forecast_.observe(session_id, units);
        congestion_.observe("default", units, units);
        links_.observe("default", 1U, 0U);
        quality_.observe("default", congestion_.score("default"));
        tariff_.quote("data", units);
        budgets_.consume(session_id, units);
        segments_.append(session_id, static_cast<std::uint32_t>(tick & 0xffffU), units);
    }
}

void OperationsFabric::observe_after(const Frame& frame, bool accepted, std::uint64_t tick) {
    const auto session_id = frame.session_id;
    const auto op = describe_operation(frame.operation);
    health_.observe("engine", "default", accepted);
    if (!accepted) advanced_.observe(frame, false, tick);
    if (!accepted) resilience_.observe(frame, false, tick);
    if (!accepted) {
        errors_.record("engine");
        links_.observe("default", 32U, 1U);
        return;
    }
    if (frame.operation == Operation::Commit) {
        snapshots_.capture(session_id, tick, {});
        schedule_.due(tick);
        audit_.record("policy");
    }
    if (frame.operation == Operation::Handover) {
        for (const auto& plan : handovers_.active(session_id)) handovers_.complete(plan.id);
    }
    if (frame.operation == Operation::Close) {
        connections_.close(session_id, tick);
        admission_.release(session_id);
        slices_.detach(session_id);
        correlations_.erase(session_id);
        identities_.remove(session_id);
        leases_.release("session:" + std::to_string(session_id), session_id);
        topology_.invalidate(session_id);
    }
    if (frame.operation == Operation::Audit) {
        const std::map<std::string, std::string> before{{"operation", op}, {"accepted", "false"}};
        const std::map<std::string, std::string> after{{"operation", op}, {"accepted", "true"}};
        (void)differ_.render(differ_.compare(before, after));
    }
    (void)queue_.pop(tick, 8U);
    (void)timers_.advance(tick);
    (void)transit_.take(4096U);
    (void)retention_.evict(tick > 256U ? tick - 256U : 0U);
    (void)idempotency_.purge(tick > 512U ? tick - 512U : 0U);
    (void)snapshots_.prune(session_id, 4U);
}

std::string OperationsFabric::report() const {
    std::vector<std::vector<std::string>> rows{{"metric", "value"}};
    rows.push_back({"connections", std::to_string(connections_.active())});
    rows.push_back({"queued", std::to_string(queue_.size())});
    rows.push_back({"commands", std::to_string(commands_.size())});
    rows.push_back({"transit", std::to_string(transit_.size())});
    rows.push_back({"uplink_sessions", std::to_string(uplink_.top(100U).size())});
    rows.push_back({"health_warnings", std::to_string(health_.unhealthy().size())});
    rows.push_back({"error_budget", std::to_string(errors_.remaining("engine"))});
    rows.push_back({"advanced", advanced_.report()});
    rows.push_back({"resilience", resilience_.report()});
    return render_table(rows);
}

void OperationsFabric::clear() {
    acl_.clear(); admission_.clear(); capacity_.clear(); congestion_.clear(); connections_.clear(); correlations_.clear();
    dependencies_.clear(); endpoints_.clear(); domains_.clear(); handovers_.clear(); idempotency_.clear(); links_.clear();
    messages_.clear(); slices_.clear(); queue_.clear(); interpreter_.clear(); snapshots_.clear(); sequences_.clear(); catalog_.clear();
    subscriptions_.clear(); telemetry_.clear(); timers_.clear(); transactions_.clear(); uplink_.clear(); workloads_.clear(); zones_.clear();
    tariff_.clear(); classifier_.clear(); replicas_.clear(); leases_.clear(); budgets_.clear(); health_.clear(); identities_.clear(); flows_.clear();
    constraints_.clear(); schedule_.clear(); audit_.clear(); errors_.clear(); retention_.clear(); versions_.clear(); quality_.clear(); transit_.clear();
    commands_.clear(); topology_.clear(); forecast_.clear(); peers_.clear(); segments_.clear(); advanced_.clear(); resilience_.clear();
}

}  // namespace aegis::detail
