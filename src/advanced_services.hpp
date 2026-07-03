#pragma once

struct WeightedEndpoint {
    std::string name;
    std::uint32_t weight{};
    std::uint64_t selected{};
    bool enabled{true};
};
class WeightedLoadBalancer {
public:
    void upsert(std::string name, std::uint32_t weight, bool enabled = true);
    std::optional<std::string> choose(std::uint64_t entropy);
    std::vector<WeightedEndpoint> endpoints() const;
    void clear();
private:
    std::unordered_map<std::string, WeightedEndpoint> endpoints_;
};

class LatencyHistogram {
public:
    void observe(std::string key, std::uint64_t value);
    std::optional<std::uint64_t> percentile(std::string_view key, std::uint32_t percent) const;
    std::map<std::uint64_t, std::uint64_t> buckets(std::string_view key) const;
    void clear();
private:
    std::unordered_map<std::string, std::vector<std::uint64_t>> values_;
};

struct ArchivedSession { std::uint32_t session_id{}; std::uint64_t generation{}; std::uint64_t tick{}; std::string state; };
class SessionArchive {
public:
    void append(std::uint32_t session_id, std::uint64_t generation, std::uint64_t tick, std::string state);
    std::optional<ArchivedSession> latest(std::uint32_t session_id) const;
    std::size_t prune(std::uint32_t session_id, std::size_t keep);
    void clear();
private:
    std::unordered_map<std::uint32_t, std::vector<ArchivedSession>> records_;
};

class CaptureReplayBuffer {
public:
    void append(PacketRecord record, std::size_t limit = 64U);
    std::vector<PacketRecord> since(std::uint64_t ordinal) const;
    std::size_t trim(std::size_t limit);
    void clear();
private:
    std::deque<PacketRecord> records_;
};

struct RouteConflict { std::uint32_t route_id{}; std::vector<std::string> discarded; };
class RouteConflictResolver {
public:
    std::pair<std::vector<Route>, std::vector<RouteConflict>> resolve(std::vector<Route> routes) const;
    std::string render(const std::vector<RouteConflict>& conflicts) const;
};

struct PeerCandidate { std::uint32_t id{}; std::string zone; std::uint64_t capacity{}; std::uint64_t active{}; bool healthy{true}; };
class PeerSelector {
public:
    void upsert(PeerCandidate candidate);
    std::optional<PeerCandidate> select(std::string_view zone) const;
    bool release(std::uint32_t id, std::uint64_t units);
    void clear();
private:
    std::unordered_map<std::uint32_t, PeerCandidate> peers_;
};

class EventWindow {
public:
    void append(JournalEntry entry, std::size_t limit = 128U);
    std::vector<JournalEntry> between(std::uint64_t from, std::uint64_t to) const;
    std::size_t count(std::string_view component) const;
    void clear();
private:
    std::deque<JournalEntry> entries_;
};

struct RetryEntry { std::string key; std::uint64_t due{}; std::uint32_t attempts{}; bool complete{}; };
class RetryPlanner {
public:
    void schedule(std::string key, std::uint64_t due);
    std::vector<RetryEntry> due(std::uint64_t tick, std::uint32_t max_attempts);
    bool complete(std::string_view key);
    void clear();
private:
    std::unordered_map<std::string, RetryEntry> entries_;
};

struct ReconciliationChange { std::string key; std::string before; std::string after; };
class StateReconciler {
public:
    std::vector<ReconciliationChange> reconcile(const std::map<std::string, std::string>& desired,
                                                 const std::map<std::string, std::string>& observed) const;
    std::map<std::string, std::string> apply(const std::map<std::string, std::string>& observed,
                                             const std::vector<ReconciliationChange>& changes) const;
};

class ConfigurationOverlay {
public:
    void set(std::string layer, std::map<std::string, std::string> values);
    std::optional<std::string> resolve(std::string_view key) const;
    std::map<std::string, std::string> merged() const;
    void clear();
private:
    std::map<std::string, std::map<std::string, std::string>> layers_;
};

class RuleSetValidator {
public:
    std::vector<std::string> validate(const std::vector<RouteRule>& rules) const;
    std::vector<RouteRule> normalize(std::vector<RouteRule> rules) const;
};

class WindowedRateEstimator {
public:
    void observe(std::string key, std::uint64_t tick, std::uint64_t units);
    std::uint64_t rate(std::string_view key, std::uint64_t from_tick) const;
    void clear();
private:
    std::unordered_map<std::string, std::deque<std::pair<std::uint64_t, std::uint64_t>>> samples_;
};

class NetworkGraphAnalyzer {
public:
    std::vector<std::uint32_t> hubs(const TopologyGraph& graph, std::size_t minimum_degree) const;
    std::vector<std::uint32_t> leaves(const TopologyGraph& graph) const;
    std::string render(const TopologyGraph& graph) const;
};

struct AuditRetentionItem { std::uint32_t session_id{}; std::uint64_t sequence{}; std::string digest; std::uint64_t tick{}; };
class AuditRetentionIndex {
public:
    void retain(AuditRetentionItem item);
    std::vector<AuditRetentionItem> query(std::uint32_t session_id) const;
    std::size_t trim(std::uint64_t before_tick);
    void clear();
private:
    std::vector<AuditRetentionItem> items_;
};

struct PolicyLifecycleEntry { std::uint32_t session_id{}; std::uint64_t version{}; std::string phase; std::uint64_t tick{}; };
class PolicyLifecycleLedger {
public:
    void note(std::uint32_t session_id, std::uint64_t version, std::string phase, std::uint64_t tick);
    std::vector<PolicyLifecycleEntry> history(std::uint32_t session_id) const;
    std::optional<PolicyLifecycleEntry> latest(std::uint32_t session_id) const;
    void clear();
private:
    std::unordered_map<std::uint32_t, std::vector<PolicyLifecycleEntry>> history_;
};

struct CongestionAdjustment { std::string path; std::uint64_t allowance{}; std::string action; };
class CongestionControlLoop {
public:
    CongestionAdjustment adjust(std::string path, std::uint64_t offered, std::uint64_t delivered, std::uint64_t allowance);
    std::optional<CongestionAdjustment> latest(std::string_view path) const;
    void clear();
private:
    std::unordered_map<std::string, CongestionAdjustment> adjustments_;
};

struct QoSProfile { std::uint32_t id{}; std::uint32_t priority{}; std::uint64_t max_burst{}; std::uint64_t latency_target{}; };
class QoSProfileStore {
public:
    void put(QoSProfile profile);
    std::optional<QoSProfile> get(std::uint32_t id) const;
    std::vector<QoSProfile> ordered() const;
    void clear();
private:
    std::unordered_map<std::uint32_t, QoSProfile> profiles_;
};

class TunnelPathResolver {
public:
    void bind(std::uint32_t tunnel_id, std::vector<std::uint32_t> path);
    std::optional<std::vector<std::uint32_t>> resolve(std::uint32_t tunnel_id) const;
    bool remove(std::uint32_t tunnel_id);
    void clear();
private:
    std::unordered_map<std::uint32_t, std::vector<std::uint32_t>> paths_;
};

class SubscriberRegistry {
public:
    void upsert(std::uint32_t session_id, std::string subscriber, std::string segment);
    std::optional<std::string> subscriber(std::uint32_t session_id) const;
    std::vector<std::uint32_t> segment(std::string_view name) const;
    void clear();
private:
    struct Entry { std::string subscriber; std::string segment; };
    std::unordered_map<std::uint32_t, Entry> entries_;
};

struct AccountingEntry { std::uint64_t id{}; std::uint32_t session_id{}; std::uint64_t units{}; std::uint64_t tick{}; bool settled{}; };
class AccountingWindow {
public:
    std::uint64_t post(std::uint32_t session_id, std::uint64_t units, std::uint64_t tick);
    bool settle(std::uint64_t id);
    std::uint64_t total(std::uint32_t session_id, bool include_unsettled = true) const;
    void clear();
private:
    std::uint64_t next_id_{1};
    std::vector<AccountingEntry> entries_;
};

class ErrorCorrelation {
public:
    void record(std::string code, std::uint32_t session_id);
    std::vector<std::pair<std::string, std::uint64_t>> top(std::size_t limit) const;
    std::vector<std::uint32_t> sessions(std::string_view code) const;
    void clear();
private:
    std::unordered_map<std::string, std::vector<std::uint32_t>> errors_;
};

class FrameFingerprintIndex {
public:
    void add(std::uint32_t session_id, std::string fingerprint);
    bool contains(std::string_view fingerprint) const;
    std::vector<std::string> for_session(std::uint32_t session_id) const;
    void clear();
private:
    std::unordered_map<std::uint32_t, std::vector<std::string>> by_session_;
    std::unordered_set<std::string> all_;
};

class MetricsRollup {
public:
    void observe(std::string metric, std::uint64_t value);
    std::uint64_t total(std::string_view metric) const;
    std::optional<std::uint64_t> average(std::string_view metric) const;
    void clear();
private:
    struct Aggregate { std::uint64_t total{}; std::uint64_t count{}; };
    std::unordered_map<std::string, Aggregate> totals_;
};

class ResourceTopology {
public:
    void connect(std::string left, std::string right);
    bool disconnect(std::string_view left, std::string_view right);
    std::vector<std::string> neighbors(std::string_view node) const;
    void clear();
private:
    std::unordered_map<std::string, std::set<std::string>> edges_;
};

class ServiceDependencyPlanner {
public:
    void register_service(std::string service, std::vector<std::string> dependencies);
    std::vector<std::string> startup_plan() const;
    std::vector<std::string> validate() const;
    void clear();
private:
    std::map<std::string, std::vector<std::string>> services_;
};

class AdvancedServiceSuite {
public:
    AdvancedServiceSuite();
    void observe(const Frame& frame, bool accepted, std::uint64_t tick);
    std::string report() const;
    void clear();
private:
    WeightedLoadBalancer balancer_;
    LatencyHistogram latency_;
    SessionArchive archive_;
    CaptureReplayBuffer replay_;
    RouteConflictResolver conflicts_;
    PeerSelector peers_;
    EventWindow events_;
    RetryPlanner retries_;
    StateReconciler reconciler_;
    ConfigurationOverlay overlay_;
    RuleSetValidator rules_;
    WindowedRateEstimator rates_;
    NetworkGraphAnalyzer graphs_;
    AuditRetentionIndex audits_;
    PolicyLifecycleLedger lifecycle_;
    CongestionControlLoop congestion_;
    QoSProfileStore qos_;
    TunnelPathResolver tunnels_;
    SubscriberRegistry subscribers_;
    AccountingWindow accounting_;
    ErrorCorrelation errors_;
    FrameFingerprintIndex fingerprints_;
    MetricsRollup metrics_;
    ResourceTopology resources_;
    ServiceDependencyPlanner planner_;
};
