#pragma once

struct AdmissionDecision {
    bool admitted{};
    std::uint32_t zone{};
    std::string reason;
};

class AccessControlList {
public:
    void grant(std::uint32_t subject, std::string capability);
    bool revoke(std::uint32_t subject, std::string_view capability);
    bool allows(std::uint32_t subject, std::string_view capability) const;
    std::vector<std::string> capabilities(std::uint32_t subject) const;
    void clear();
private:
    std::unordered_map<std::uint32_t, std::set<std::string>> grants_;
};

class AdmissionController {
public:
    void configure(std::uint32_t zone, std::size_t limit);
    AdmissionDecision admit(std::uint32_t session_id, std::uint32_t zone);
    bool release(std::uint32_t session_id);
    std::size_t active(std::uint32_t zone) const;
    void clear();
private:
    std::unordered_map<std::uint32_t, std::size_t> limits_;
    std::unordered_map<std::uint32_t, std::uint32_t> sessions_;
};

class CapacityPool {
public:
    void set_capacity(std::string resource, std::uint64_t capacity);
    bool reserve(std::string_view resource, std::uint64_t amount);
    void release(std::string_view resource, std::uint64_t amount);
    std::uint64_t available(std::string_view resource) const;
    std::string report() const;
    void clear();
private:
    struct Slot { std::uint64_t capacity{}; std::uint64_t used{}; };
    std::unordered_map<std::string, Slot> slots_;
};

class CongestionEstimator {
public:
    void observe(std::string path, std::uint64_t offered, std::uint64_t delivered);
    std::uint64_t score(std::string_view path) const;
    std::string advice(std::string_view path) const;
    void clear();
private:
    struct Sample { std::uint64_t offered{}; std::uint64_t delivered{}; std::uint64_t samples{}; };
    std::unordered_map<std::string, Sample> samples_;
};

class ConnectionLedger {
public:
    void open(std::uint32_t session_id, std::string endpoint, std::uint64_t tick);
    void close(std::uint32_t session_id, std::uint64_t tick);
    std::optional<std::string> endpoint(std::uint32_t session_id) const;
    std::size_t active() const;
    std::string report() const;
    void clear();
private:
    struct Entry { std::string endpoint; std::uint64_t opened{}; std::uint64_t closed{}; bool live{}; };
    std::unordered_map<std::uint32_t, Entry> entries_;
};

class CorrelationRegistry {
public:
    void link(std::string key, std::uint32_t session_id);
    std::vector<std::uint32_t> lookup(std::string_view key) const;
    std::size_t erase(std::uint32_t session_id);
    std::string report() const;
    void clear();
private:
    std::unordered_map<std::string, std::vector<std::uint32_t>> links_;
};

class DependencyGraph {
public:
    void add(std::string component, std::string dependency);
    bool remove(std::string_view component, std::string_view dependency);
    std::vector<std::string> closure(std::string_view component) const;
    bool cyclic(std::string_view component) const;
    void clear();
private:
    std::unordered_map<std::string, std::set<std::string>> edges_;
};

class EndpointDirectory {
public:
    void upsert(std::string name, std::string address, std::uint32_t weight);
    std::optional<std::string> resolve(std::string_view name) const;
    bool remove(std::string_view name);
    std::vector<std::string> names() const;
    void clear();
private:
    struct Endpoint { std::string address; std::uint32_t weight{}; };
    std::unordered_map<std::string, Endpoint> endpoints_;
};

class FailureDomainTable {
public:
    void assign(std::uint32_t session_id, std::string domain);
    std::vector<std::uint32_t> members(std::string_view domain) const;
    void mark(std::string domain, bool healthy);
    bool healthy(std::string_view domain) const;
    void clear();
private:
    std::unordered_map<std::uint32_t, std::string> assignments_;
    std::unordered_map<std::string, bool> health_;
};

struct HandoverPlan {
    std::uint64_t id{};
    std::uint32_t session_id{};
    std::uint32_t source{};
    std::uint32_t destination{};
    std::string state;
};

class HandoverPlanner {
public:
    std::uint64_t propose(std::uint32_t session_id, std::uint32_t source, std::uint32_t destination);
    bool approve(std::uint64_t id);
    bool complete(std::uint64_t id);
    std::vector<HandoverPlan> active(std::uint32_t session_id) const;
    void clear();
private:
    std::uint64_t next_id_{1};
    std::unordered_map<std::uint64_t, HandoverPlan> plans_;
};

class IdempotencyLedger {
public:
    bool record(std::uint32_t session_id, std::string key, std::uint64_t tick);
    bool seen(std::uint32_t session_id, std::string_view key) const;
    std::size_t purge(std::uint64_t before_tick);
    void clear();
private:
    std::unordered_map<std::string, std::uint64_t> entries_;
};

class LinkMonitor {
public:
    void observe(std::string link, std::uint64_t latency, std::uint64_t loss);
    std::optional<std::pair<std::uint64_t, std::uint64_t>> status(std::string_view link) const;
    std::vector<std::string> degraded(std::uint64_t latency_limit, std::uint64_t loss_limit) const;
    void clear();
private:
    std::unordered_map<std::string, std::pair<std::uint64_t, std::uint64_t>> links_;
};

class MessageCatalog {
public:
    void record(Operation operation);
    std::uint64_t count(Operation operation) const;
    std::string summary() const;
    void clear();
private:
    std::map<Operation, std::uint64_t> counts_;
};

class NetworkSliceRegistry {
public:
    void attach(std::uint32_t session_id, std::string slice);
    bool detach(std::uint32_t session_id);
    std::optional<std::string> slice(std::uint32_t session_id) const;
    std::vector<std::uint32_t> members(std::string_view slice) const;
    void clear();
private:
    std::unordered_map<std::uint32_t, std::string> memberships_;
};

struct QueuedOperation {
    std::uint64_t id{};
    std::uint64_t due{};
    std::uint32_t session_id{};
    std::string kind;
    bool cancelled{};
};

class OperationQueue {
public:
    std::uint64_t push(std::uint64_t due, std::uint32_t session_id, std::string kind);
    std::vector<QueuedOperation> pop(std::uint64_t tick, std::size_t limit);
    bool cancel(std::uint64_t id);
    std::size_t size() const;
    void clear();
private:
    std::uint64_t next_id_{1};
    std::vector<QueuedOperation> items_;
};

class PolicyInterpreter {
public:
    void load(std::uint32_t session_id, std::vector<RouteRule> rules);
    std::optional<std::uint32_t> evaluate(std::uint32_t session_id, std::string_view payload) const;
    std::size_t rule_count(std::uint32_t session_id) const;
    void clear();
private:
    std::unordered_map<std::uint32_t, std::vector<RouteRule>> rules_;
};

struct RoutingSnapshot {
    std::uint64_t id{};
    std::uint32_t session_id{};
    std::uint64_t generation{};
    std::vector<Route> routes;
};

class RoutingSnapshotStore {
public:
    std::uint64_t capture(std::uint32_t session_id, std::uint64_t generation, std::vector<Route> routes);
    std::optional<RoutingSnapshot> latest(std::uint32_t session_id) const;
    std::size_t prune(std::uint32_t session_id, std::size_t keep);
    void clear();
private:
    std::uint64_t next_id_{1};
    std::unordered_map<std::uint32_t, std::vector<RoutingSnapshot>> history_;
};

class SequenceTracker {
public:
    void observe(std::uint32_t session_id, std::uint32_t sequence);
    std::uint64_t gap_count(std::uint32_t session_id) const;
    std::uint32_t next_expected(std::uint32_t session_id) const;
    void clear();
private:
    struct State { std::uint32_t last{}; std::uint64_t gaps{}; bool initialized{}; };
    std::unordered_map<std::uint32_t, State> states_;
};

class ServiceCatalog {
public:
    void register_service(std::string service, std::string version, std::vector<std::string> capabilities);
    std::optional<std::string> locate(std::string_view capability) const;
    bool remove(std::string_view service);
    std::vector<std::string> services() const;
    void clear();
private:
    struct Entry { std::string version; std::vector<std::string> capabilities; };
    std::unordered_map<std::string, Entry> services_;
};

struct StateChange { std::string key; std::string before; std::string after; };
class StateDiffer {
public:
    std::vector<StateChange> compare(const std::map<std::string, std::string>& before,
                                     const std::map<std::string, std::string>& after) const;
    std::string render(const std::vector<StateChange>& changes) const;
};

class SubscriptionRegistry {
public:
    void subscribe(std::uint32_t session_id, std::string topic);
    bool unsubscribe(std::uint32_t session_id, std::string_view topic);
    std::vector<std::string> topics(std::uint32_t session_id) const;
    std::vector<std::uint32_t> subscribers(std::string_view topic) const;
    void clear();
private:
    std::unordered_map<std::uint32_t, std::set<std::string>> subscriptions_;
};

class TelemetrySampler {
public:
    void sample(std::string metric, std::uint64_t value, std::size_t limit = 32U);
    std::optional<std::uint64_t> average(std::string_view metric) const;
    std::vector<std::uint64_t> series(std::string_view metric) const;
    void clear();
private:
    std::unordered_map<std::string, std::deque<std::uint64_t>> samples_;
};

struct TimerEvent { std::uint64_t id{}; std::uint64_t due{}; std::string key; bool cancelled{}; };
class TimerWheel {
public:
    std::uint64_t schedule(std::uint64_t due, std::string key);
    std::vector<TimerEvent> advance(std::uint64_t tick);
    bool cancel(std::uint64_t id);
    void clear();
private:
    std::uint64_t next_id_{1};
    std::vector<TimerEvent> events_;
};

struct TransactionNode { std::uint64_t id{}; std::string label; bool complete{}; std::vector<std::uint64_t> parents; };
class TransactionGraph {
public:
    std::uint64_t begin(std::string label);
    bool link(std::uint64_t child, std::uint64_t parent);
    bool complete(std::uint64_t id);
    std::vector<TransactionNode> pending() const;
    void clear();
private:
    std::uint64_t next_id_{1};
    std::unordered_map<std::uint64_t, TransactionNode> nodes_;
};

class UplinkAggregator {
public:
    void add(std::uint32_t session_id, std::uint64_t units);
    std::uint64_t total(std::uint32_t session_id) const;
    std::vector<std::pair<std::uint32_t, std::uint64_t>> top(std::size_t limit) const;
    void clear();
private:
    std::unordered_map<std::uint32_t, std::uint64_t> totals_;
};

class WorkloadClassifier {
public:
    std::string classify(std::span<const Byte> payload) const;
    void observe(std::string category);
    std::map<std::string, std::uint64_t> histogram() const;
    void clear();
private:
    std::map<std::string, std::uint64_t> counts_;
};

class ZoneRegistry {
public:
    void assign(std::uint32_t session_id, std::string zone);
    std::optional<std::string> zone(std::uint32_t session_id) const;
    std::vector<std::uint32_t> occupants(std::string_view zone) const;
    void clear();
private:
    std::unordered_map<std::uint32_t, std::string> zones_;
};

class ChargingTariff {
public:
    void set_rate(std::string category, std::uint64_t micros_per_unit);
    std::uint64_t quote(std::string_view category, std::uint64_t units) const;
    std::vector<std::string> categories() const;
    void clear();
private:
    std::unordered_map<std::string, std::uint64_t> rates_;
};

class PacketClassifier {
public:
    void set_weight(std::string label, std::uint32_t weight);
    std::string classify(const Frame& frame) const;
    std::uint32_t weight(std::string_view label) const;
    void clear();
private:
    std::unordered_map<std::string, std::uint32_t> weights_;
};

class ReplicaSetPlanner {
public:
    void plan(std::uint32_t session_id, std::vector<std::string> replicas);
    std::optional<std::string> promote(std::uint32_t session_id);
    std::vector<std::string> members(std::uint32_t session_id) const;
    void clear();
private:
    std::unordered_map<std::uint32_t, std::deque<std::string>> replicas_;
};

struct ResourceLease { std::string key; std::uint32_t holder{}; std::uint64_t expires{}; };
class ResourceLeaseTable {
public:
    bool acquire(std::string key, std::uint32_t holder, std::uint64_t expires);
    bool renew(std::string_view key, std::uint32_t holder, std::uint64_t expires);
    bool release(std::string_view key, std::uint32_t holder);
    std::vector<ResourceLease> active(std::uint64_t tick) const;
    void clear();
private:
    std::unordered_map<std::string, ResourceLease> leases_;
};

class SessionBudgetPlanner {
public:
    void configure(std::uint32_t session_id, std::uint64_t budget);
    bool consume(std::uint32_t session_id, std::uint64_t units);
    std::uint64_t remaining(std::uint32_t session_id) const;
    void clear();
private:
    std::unordered_map<std::uint32_t, std::uint64_t> remaining_;
};

class ServiceHealthMatrix {
public:
    void observe(std::string service, std::string zone, bool healthy);
    bool healthy(std::string_view service, std::string_view zone) const;
    std::vector<std::string> unhealthy() const;
    void clear();
private:
    std::map<std::pair<std::string, std::string>, bool> matrix_;
};

class IdentityCorrelation {
public:
    void observe(std::uint32_t session_id, std::string subscriber, std::string endpoint);
    std::vector<std::uint32_t> related(std::string_view subscriber) const;
    std::size_t remove(std::uint32_t session_id);
    void clear();
private:
    struct Item { std::string subscriber; std::string endpoint; };
    std::unordered_map<std::uint32_t, Item> items_;
};

class FlowAggregation {
public:
    void add(std::uint32_t session_id, std::string flow, std::uint64_t units);
    std::uint64_t aggregate(std::string_view flow) const;
    std::vector<std::pair<std::string, std::uint64_t>> top(std::size_t limit) const;
    void clear();
private:
    std::unordered_map<std::string, std::uint64_t> flows_;
};

class RoutingConstraints {
public:
    void set(std::uint32_t session_id, std::vector<std::string> blocked);
    bool allows(std::uint32_t session_id, std::string_view route) const;
    std::vector<std::string> blocked(std::uint32_t session_id) const;
    void clear();
private:
    std::unordered_map<std::uint32_t, std::set<std::string>> blocked_;
};

struct ScheduledPolicy { std::uint64_t id{}; std::uint32_t session_id{}; std::uint64_t due{}; std::string label; };
class PolicySchedule {
public:
    std::uint64_t schedule(std::uint32_t session_id, std::uint64_t due, std::string label);
    std::vector<ScheduledPolicy> due(std::uint64_t tick);
    void clear();
private:
    std::uint64_t next_id_{1};
    std::vector<ScheduledPolicy> entries_;
};

class AuditFilter {
public:
    void allow(std::string component);
    bool accepts(std::string_view component) const;
    void record(std::string component);
    std::string summary() const;
    void clear();
private:
    std::set<std::string> allowed_;
    std::map<std::string, std::uint64_t> observed_;
};

class ErrorBudget {
public:
    void configure(std::string scope, std::uint64_t allowance);
    bool record(std::string_view scope, std::uint64_t amount = 1U);
    std::uint64_t remaining(std::string_view scope) const;
    void clear();
private:
    std::unordered_map<std::string, std::uint64_t> remaining_;
};

class RetentionPlanner {
public:
    void remember(std::string key, std::uint64_t tick);
    std::vector<std::string> evict(std::uint64_t before_tick);
    std::vector<std::string> keys() const;
    void clear();
private:
    std::map<std::string, std::uint64_t> entries_;
};

class ProtocolVersionRegistry {
public:
    void register_version(std::string name, std::uint32_t min, std::uint32_t max);
    bool supports(std::string_view name, std::uint32_t version) const;
    std::vector<std::string> versions() const;
    void clear();
private:
    std::unordered_map<std::string, std::pair<std::uint32_t, std::uint32_t>> ranges_;
};

class PathQualityLedger {
public:
    void observe(std::string path, std::uint64_t score);
    std::optional<std::string> best() const;
    std::vector<std::pair<std::string, std::uint64_t>> history() const;
    void clear();
private:
    std::map<std::string, std::uint64_t> scores_;
};

class TransitWindow {
public:
    void offer(std::uint32_t session_id, std::uint64_t units);
    std::vector<std::pair<std::uint32_t, std::uint64_t>> take(std::uint64_t limit);
    std::size_t size() const;
    void clear();
private:
    std::deque<std::pair<std::uint32_t, std::uint64_t>> pending_;
};

class CommandDeduplicator {
public:
    bool accept(std::uint32_t session_id, std::string command, std::uint64_t tick);
    std::size_t size() const;
    void clear();
private:
    std::unordered_map<std::string, std::uint64_t> seen_;
};

class TopologyCache {
public:
    void put(std::uint32_t session_id, TopologyGraph graph);
    std::optional<TopologyGraph> get(std::uint32_t session_id) const;
    bool invalidate(std::uint32_t session_id);
    void clear();
private:
    std::unordered_map<std::uint32_t, TopologyGraph> graphs_;
};

class UsageForecast {
public:
    void observe(std::uint32_t session_id, std::uint64_t units);
    std::uint64_t predict(std::uint32_t session_id) const;
    std::string report() const;
    void clear();
private:
    std::unordered_map<std::uint32_t, std::deque<std::uint64_t>> samples_;
};

class PeerDirectory {
public:
    void register_peer(std::uint32_t id, std::string endpoint, std::string zone);
    std::optional<std::string> resolve(std::uint32_t id) const;
    bool remove(std::uint32_t id);
    std::vector<std::uint32_t> in_zone(std::string_view zone) const;
    void clear();
private:
    struct Peer { std::string endpoint; std::string zone; };
    std::unordered_map<std::uint32_t, Peer> peers_;
};

class SegmentLedger {
public:
    void append(std::uint32_t session_id, std::uint32_t segment, std::uint64_t units);
    std::string summary(std::uint32_t session_id) const;
    std::size_t trim(std::uint32_t session_id, std::size_t limit);
    void clear();
private:
    std::unordered_map<std::uint32_t, std::vector<std::pair<std::uint32_t, std::uint64_t>>> entries_;
};

class OperationsFabric {
public:
    OperationsFabric();
    void observe_before(const Frame& frame, std::uint64_t tick);
    void observe_after(const Frame& frame, bool accepted, std::uint64_t tick);
    std::string report() const;
    void clear();
private:
    AccessControlList acl_;
    AdmissionController admission_;
    CapacityPool capacity_;
    CongestionEstimator congestion_;
    ConnectionLedger connections_;
    CorrelationRegistry correlations_;
    DependencyGraph dependencies_;
    EndpointDirectory endpoints_;
    FailureDomainTable domains_;
    HandoverPlanner handovers_;
    IdempotencyLedger idempotency_;
    LinkMonitor links_;
    MessageCatalog messages_;
    NetworkSliceRegistry slices_;
    OperationQueue queue_;
    PolicyInterpreter interpreter_;
    RoutingSnapshotStore snapshots_;
    SequenceTracker sequences_;
    ServiceCatalog catalog_;
    StateDiffer differ_;
    SubscriptionRegistry subscriptions_;
    TelemetrySampler telemetry_;
    TimerWheel timers_;
    TransactionGraph transactions_;
    UplinkAggregator uplink_;
    WorkloadClassifier workloads_;
    ZoneRegistry zones_;
    ChargingTariff tariff_;
    PacketClassifier classifier_;
    ReplicaSetPlanner replicas_;
    ResourceLeaseTable leases_;
    SessionBudgetPlanner budgets_;
    ServiceHealthMatrix health_;
    IdentityCorrelation identities_;
    FlowAggregation flows_;
    RoutingConstraints constraints_;
    PolicySchedule schedule_;
    AuditFilter audit_;
    ErrorBudget errors_;
    RetentionPlanner retention_;
    ProtocolVersionRegistry versions_;
    PathQualityLedger quality_;
    TransitWindow transit_;
    CommandDeduplicator commands_;
    TopologyCache topology_;
    UsageForecast forecast_;
    PeerDirectory peers_;
    SegmentLedger segments_;
    AdvancedServiceSuite advanced_;
    ResilienceServiceSuite resilience_;
};
