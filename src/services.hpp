#pragma once


struct QuotaReservation {
    std::uint64_t token{};
    std::uint32_t session_id{};
    std::uint64_t requested{};
    std::uint64_t granted{};
    std::uint64_t generation{};
    std::uint64_t issued_tick{};
    bool settled{};
};

struct QuotaAccount {
    std::uint64_t ceiling{};
    std::uint64_t available{};
    std::uint64_t burst{};
    std::uint64_t consumed{};
    std::uint64_t rejected{};
    std::uint64_t generation{};
};

class QuotaManager {
public:
    void configure(std::uint32_t session_id, std::uint64_t ceiling, std::uint64_t burst,
                   std::uint64_t generation);
    std::optional<QuotaReservation> reserve(std::uint32_t session_id, std::uint64_t requested,
                                            std::uint64_t generation);
    bool settle(std::uint64_t token, std::uint64_t actual);
    bool release(std::uint64_t token);
    void rollover(std::uint32_t session_id, std::uint64_t amount);
    std::optional<QuotaAccount> account(std::uint32_t session_id) const;
    std::vector<QuotaReservation> outstanding(std::uint32_t session_id) const;
    std::size_t expire(std::uint64_t tick, std::uint64_t max_age);
    std::string report() const;
    void clear();
private:
    std::uint64_t next_token_{1};
    std::uint64_t tick_{};
    std::unordered_map<std::uint32_t, QuotaAccount> accounts_;
    std::unordered_map<std::uint64_t, QuotaReservation> reservations_;
};

struct ChargeLine {
    std::uint64_t id{};
    std::uint32_t session_id{};
    std::uint64_t generation{};
    std::uint64_t units{};
    std::uint64_t amount_micros{};
    std::string category;
    std::string reference;
    bool reversed{};
};

class ChargingLedger {
public:
    std::uint64_t post(std::uint32_t session_id, std::uint64_t generation, std::uint64_t units,
                       std::string category, std::string reference);
    bool reverse(std::uint64_t line_id, std::string reason);
    std::uint64_t total(std::uint32_t session_id) const;
    std::uint64_t total_by_category(std::string_view category) const;
    std::vector<ChargeLine> lines(std::uint32_t session_id) const;
    std::vector<ChargeLine> reconcile(std::uint32_t session_id, std::uint64_t generation) const;
    std::string receipt(std::uint32_t session_id) const;
    void clear();
private:
    std::uint64_t next_line_{1};
    std::vector<ChargeLine> lines_;
    std::unordered_map<std::string, std::uint64_t> reference_index_;
};

struct WorkItem {
    std::uint64_t id{};
    std::uint32_t session_id{};
    std::uint32_t priority{};
    std::uint64_t due_tick{};
    std::uint64_t generation{};
    std::string kind;
    std::string payload;
    bool cancelled{};
};

class PriorityScheduler {
public:
    std::uint64_t enqueue(std::uint32_t session_id, std::uint32_t priority, std::uint64_t delay,
                          std::uint64_t generation, std::string kind, std::string payload);
    std::vector<WorkItem> take_ready(std::uint64_t tick, std::size_t limit);
    bool cancel(std::uint64_t id);
    std::size_t cancel_session(std::uint32_t session_id);
    void rebalance(std::uint32_t session_id, std::uint32_t floor, std::uint32_t ceiling);
    std::vector<WorkItem> pending(std::uint32_t session_id) const;
    std::string report() const;
    void clear();
private:
    std::uint64_t next_id_{1};
    std::vector<WorkItem> items_;
};

enum class MobilityPhase : Byte { Prepared = 0, Admitted = 1, Transferred = 2, Completed = 3, Aborted = 4 };

struct MobilityRecord {
    std::uint64_t id{};
    std::uint32_t session_id{};
    std::uint32_t origin{};
    std::uint32_t destination{};
    std::uint64_t generation{};
    MobilityPhase phase{MobilityPhase::Prepared};
    std::vector<std::string> checkpoints;
    std::string reason;
};

class MobilityTracker {
public:
    std::uint64_t begin(std::uint32_t session_id, std::uint32_t origin, std::uint32_t destination,
                        std::uint64_t generation);
    bool admit(std::uint64_t id, std::string checkpoint);
    bool transfer(std::uint64_t id, std::string checkpoint);
    bool complete(std::uint64_t id, std::string confirmation);
    bool abort(std::uint64_t id, std::string reason);
    std::optional<MobilityRecord> find(std::uint64_t id) const;
    std::vector<MobilityRecord> active(std::uint32_t session_id) const;
    std::size_t close_generation(std::uint32_t session_id, std::uint64_t generation);
    std::string report() const;
    void clear();
private:
    std::uint64_t next_id_{1};
    std::unordered_map<std::uint64_t, MobilityRecord> records_;
};

struct JournalEntry {
    std::uint64_t sequence{};
    std::uint64_t tick{};
    std::uint32_t session_id{};
    std::string component;
    std::string action;
    std::string payload;
    std::string digest;
};

class EventJournal {
public:
    std::uint64_t append(std::uint32_t session_id, std::string component, std::string action,
                         std::string payload);
    std::vector<JournalEntry> query(std::uint32_t session_id, std::string_view component = {}) const;
    std::vector<JournalEntry> tail(std::size_t limit) const;
    std::vector<JournalEntry> since(std::uint64_t sequence) const;
    std::size_t compact(std::uint64_t keep_from);
    std::string encode() const;
    bool decode_append(std::string_view text);
    std::string report() const;
    void clear();
private:
    std::uint64_t next_sequence_{1};
    std::uint64_t tick_{};
    std::vector<JournalEntry> entries_;
};

struct Checkpoint {
    std::uint64_t id{};
    std::uint32_t session_id{};
    std::uint64_t generation{};
    std::string reason;
    std::string state;
    std::string digest;
    std::uint64_t captured_tick{};
};

class CheckpointManager {
public:
    std::uint64_t capture(std::uint32_t session_id, std::uint64_t generation, std::string reason,
                          std::string state);
    std::optional<Checkpoint> latest(std::uint32_t session_id) const;
    std::vector<Checkpoint> history(std::uint32_t session_id) const;
    std::vector<Checkpoint> all() const;
    bool verify(std::uint64_t id) const;
    std::size_t prune(std::uint32_t session_id, std::size_t limit);
    std::string export_session(std::uint32_t session_id) const;
    std::string report() const;
    void clear();
private:
    std::uint64_t next_id_{1};
    std::uint64_t tick_{};
    std::vector<Checkpoint> points_;
};

struct HealthSignal {
    std::uint64_t tick{};
    std::string source;
    std::string name;
    std::int64_t value{};
    std::string detail;
};

struct HealthStatus {
    std::string component;
    std::string state;
    std::int64_t score{};
    std::vector<std::string> reasons;
};

class HealthMonitor {
public:
    void observe(std::string source, std::string name, std::int64_t value, std::string detail = {});
    void mark(std::string component, bool healthy, std::string reason);
    HealthStatus evaluate(std::string_view component) const;
    std::vector<HealthStatus> all() const;
    std::vector<HealthSignal> recent(std::size_t limit) const;
    std::string report() const;
    void clear();
private:
    std::uint64_t tick_{};
    std::vector<HealthSignal> signals_;
    std::unordered_map<std::string, std::vector<std::pair<bool, std::string>>> marks_;
};

struct FragmentKey {
    std::uint32_t stream{};
    std::uint32_t message{};
    bool operator==(const FragmentKey& other) const { return stream == other.stream && message == other.message; }
};

struct FragmentKeyHash {
    std::size_t operator()(const FragmentKey& key) const {
        return (static_cast<std::size_t>(key.stream) << 32U) ^ key.message;
    }
};

struct FragmentPart {
    std::uint32_t offset{};
    Bytes bytes;
    bool terminal{};
};

class StreamReassembler {
public:
    bool insert(FragmentKey key, std::uint32_t offset, std::span<const Byte> bytes, bool terminal);
    std::optional<Bytes> take(FragmentKey key);
    std::size_t discard_before(std::uint64_t tick_limit);
    std::size_t pending() const;
    std::string report() const;
    void clear();
private:
    struct Assembly {
        std::uint64_t tick{};
        std::vector<FragmentPart> parts;
        std::optional<std::uint32_t> total;
    };
    std::uint64_t tick_{};
    std::unordered_map<FragmentKey, Assembly, FragmentKeyHash> assemblies_;
};

struct RuleAtom {
    std::string key;
    std::string op;
    std::string value;
};

struct RuleDecision {
    bool matched{};
    std::int64_t score{};
    std::vector<std::string> evidence;
};

class RuleEvaluator {
public:
    std::vector<RuleAtom> parse(std::string_view expression) const;
    RuleDecision evaluate(const std::vector<RuleAtom>& atoms,
                          const std::unordered_map<std::string, std::string>& facts) const;
    RuleDecision evaluate_policy(const CompiledPolicy* policy, const Session& session,
                                 std::string_view payload) const;
    std::string explain(const RuleDecision& decision) const;
};

struct RetentionItem {
    std::string key;
    std::string category;
    std::string value;
    std::uint64_t created{};
    std::uint64_t expires{};
};

class RetentionStore {
public:
    void put(std::string key, std::string category, std::string value, std::uint64_t ttl);
    std::optional<RetentionItem> get(std::string_view key) const;
    std::vector<RetentionItem> find(std::string_view category) const;
    std::size_t expire(std::uint64_t advance);
    std::size_t size() const;
    std::string report() const;
    void clear();
private:
    std::uint64_t tick_{};
    std::unordered_map<std::string, RetentionItem> items_;
};

struct TransactionStep {
    std::string participant;
    std::string operation;
    bool applied{};
    std::string result;
};

struct TransactionRecord {
    std::uint64_t id{};
    std::uint32_t session_id{};
    std::uint64_t generation{};
    std::string name;
    std::vector<TransactionStep> steps;
    bool committed{};
    bool rolled_back{};
};

class TransactionCoordinator {
public:
    std::uint64_t begin(std::uint32_t session_id, std::uint64_t generation, std::string name);
    bool add_step(std::uint64_t id, std::string participant, std::string operation);
    bool apply(std::uint64_t id, std::string participant, std::string result);
    bool commit(std::uint64_t id);
    bool rollback(std::uint64_t id, std::string reason);
    std::optional<TransactionRecord> record(std::uint64_t id) const;
    std::vector<TransactionRecord> history(std::uint32_t session_id) const;
    std::string report() const;
    void clear();
private:
    std::uint64_t next_id_{1};
    std::unordered_map<std::uint64_t, TransactionRecord> records_;
};

struct ConfigRevision {
    std::uint64_t version{};
    std::unordered_map<std::string, std::string> values;
    std::string digest;
};

class ConfigurationCatalog {
public:
    std::uint64_t publish(std::unordered_map<std::string, std::string> values);
    std::optional<std::string> get(std::uint64_t version, std::string_view key) const;
    std::optional<ConfigRevision> revision(std::uint64_t version) const;
    std::vector<std::string> validate(const std::unordered_map<std::string, std::string>& values) const;
    std::uint64_t current() const;
    std::string report() const;
    void clear();
private:
    std::uint64_t next_version_{1};
    std::unordered_map<std::uint64_t, ConfigRevision> revisions_;
};

struct UsageBucket {
    std::uint32_t session_id{};
    std::uint64_t generation{};
    std::uint64_t messages{};
    std::uint64_t payload_bytes{};
    std::uint64_t decisions{};
    std::uint64_t rejected{};
    std::set<std::string> route_keys;
};

class UsageAggregator {
public:
    void record(std::uint32_t session_id, std::uint64_t generation, std::size_t payload_bytes,
                std::string route_key, bool accepted);
    std::optional<UsageBucket> bucket(std::uint32_t session_id) const;
    std::vector<UsageBucket> all() const;
    std::string report() const;
    void clear();
private:
    std::unordered_map<std::uint32_t, UsageBucket> buckets_;
};

struct ReportSection {
    std::string name;
    std::vector<std::pair<std::string, std::string>> rows;
};

class ReportAssembler {
public:
    void section(std::string name);
    void field(std::string key, std::string value);
    void field(std::string key, std::uint64_t value);
    std::string text() const;
    std::string json() const;
    void clear();
private:
    std::vector<ReportSection> sections_;
};

class IngressFilter {
public:
    bool allow(const Frame& frame, std::size_t raw_size, std::string& reason);
    void observe_result(const Frame& frame, bool accepted);
    std::size_t rejected() const;
    std::string report() const;
    void clear();
private:
    struct SourceWindow { std::uint64_t bytes{}; std::uint64_t accepted{}; std::uint64_t rejected{}; };
    std::unordered_map<std::uint32_t, SourceWindow> sources_;
    std::size_t rejected_{};
};



struct RateBucket {
    double tokens{};
    double capacity{};
    double refill_per_tick{};
    std::uint64_t last_tick{};
};

class RateLimiter {
public:
    void configure(std::string key, double capacity, double refill_per_tick);
    bool consume(std::string_view key, double cost, std::uint64_t tick);
    std::optional<RateBucket> bucket(std::string_view key) const;
    std::string report() const;
    void clear();
private:
    std::unordered_map<std::string, RateBucket> buckets_;
};

struct BreakerState {
    std::uint32_t failures{};
    std::uint32_t successes{};
    std::uint64_t opened_until{};
    bool open{};
};

class CircuitBreaker {
public:
    bool allow(std::string_view key, std::uint64_t tick) const;
    void record_success(std::string key);
    void record_failure(std::string key, std::uint64_t tick, std::uint32_t threshold, std::uint64_t cooldown);
    std::optional<BreakerState> state(std::string_view key) const;
    std::string report() const;
    void clear();
private:
    std::unordered_map<std::string, BreakerState> states_;
};

struct TenantProfile {
    std::uint32_t id{};
    std::string name;
    std::uint64_t quota{};
    std::set<std::string> capabilities;
    bool enabled{true};
};

class TenantRegistry {
public:
    bool register_tenant(TenantProfile profile);
    std::optional<TenantProfile> find(std::uint32_t id) const;
    bool enable(std::uint32_t id, bool enabled);
    std::vector<TenantProfile> list() const;
    std::vector<std::string> validate(std::uint32_t id, std::string_view capability) const;
    std::string report() const;
    void clear();
private:
    std::unordered_map<std::uint32_t, TenantProfile> tenants_;
};

struct ErrorDescriptor {
    std::string code;
    std::string category;
    std::string severity;
    std::string remediation;
};

class ErrorCatalog {
public:
    ErrorCatalog();
    std::optional<ErrorDescriptor> find(std::string_view code) const;
    std::vector<ErrorDescriptor> by_category(std::string_view category) const;
    std::string describe(std::string_view code) const;
private:
    std::unordered_map<std::string, ErrorDescriptor> entries_;
};

struct IntegrityCheck {
    std::string name;
    bool passed{};
    std::string detail;
};

class IntegrityVerifier {
public:
    std::vector<IntegrityCheck> session(const Session& session) const;
    std::vector<IntegrityCheck> policy(const CompiledPolicy* policy) const;
    std::vector<IntegrityCheck> topology(const TopologyGraph& graph) const;
    std::string summarize(const std::vector<IntegrityCheck>& checks) const;
};

struct MaintenanceTask {
    std::uint64_t id{};
    std::string component;
    std::string action;
    std::uint64_t due{};
    std::uint64_t interval{};
    bool recurring{};
    bool cancelled{};
};

class MaintenancePlanner {
public:
    std::uint64_t schedule(std::string component, std::string action, std::uint64_t due,
                           std::uint64_t interval = 0U);
    std::vector<MaintenanceTask> due(std::uint64_t tick, std::size_t limit);
    bool cancel(std::uint64_t id);
    std::vector<MaintenanceTask> pending() const;
    std::string report() const;
    void clear();
private:
    std::uint64_t next_id_{1};
    std::vector<MaintenanceTask> tasks_;
};

struct StatisticSeries {
    std::uint64_t count{};
    std::uint64_t sum{};
    std::uint64_t min{std::numeric_limits<std::uint64_t>::max()};
    std::uint64_t max{};
    std::array<std::uint64_t, 8> buckets{};
};

class StatisticsRegistry {
public:
    void observe(std::string name, std::uint64_t value);
    void increment(std::string name, std::uint64_t amount = 1U);
    std::optional<StatisticSeries> series(std::string_view name) const;
    std::string report() const;
    void clear();
private:
    std::unordered_map<std::string, StatisticSeries> series_;
};

struct TimelineEvent {
    std::uint64_t sequence{};
    std::uint32_t session_id{};
    std::uint64_t generation{};
    std::string kind;
    std::string detail;
};

class SessionTimeline {
public:
    void append(std::uint32_t session_id, std::uint64_t generation, std::string kind, std::string detail);
    std::vector<TimelineEvent> events(std::uint32_t session_id) const;
    std::vector<TimelineEvent> between(std::uint64_t first, std::uint64_t last) const;
    std::string render(std::uint32_t session_id) const;
    void clear();
private:
    std::uint64_t next_sequence_{1};
    std::vector<TimelineEvent> entries_;
};

struct SnapshotDescriptor {
    std::uint64_t id{};
    std::uint32_t session_id{};
    std::uint64_t generation{};
    std::string label;
    std::uint64_t bytes{};
    std::string checksum;
};

class SnapshotCatalog {
public:
    std::uint64_t record(std::uint32_t session_id, std::uint64_t generation, std::string label,
                         std::span<const Byte> bytes);
    std::vector<SnapshotDescriptor> list(std::uint32_t session_id) const;
    std::optional<SnapshotDescriptor> latest(std::uint32_t session_id) const;
    std::size_t erase_before(std::uint64_t id);
    std::string report() const;
    void clear();
private:
    std::uint64_t next_id_{1};
    std::vector<SnapshotDescriptor> descriptors_;
};

struct RouteDecision {
    std::uint32_t route_id{};
    std::string next_hop;
    std::uint64_t score{};
    std::vector<std::string> reasons;
};

class RoutePlanner {
public:
    std::optional<RouteDecision> select(const Session& session, const CompiledPolicy* policy,
                                        std::string_view requested_label) const;
    std::vector<RouteDecision> rank(const Session& session, const CompiledPolicy* policy) const;
    std::string explain(const RouteDecision& decision) const;
};

struct AuditEnvelope {
    std::uint64_t id{};
    std::uint32_t session_id{};
    std::string actor;
    std::string action;
    std::string target;
    std::string fingerprint;
    bool redacted{};
};

class AuditPipeline {
public:
    std::uint64_t submit(std::uint32_t session_id, std::string actor, std::string action,
                         std::string target, bool redact);
    std::vector<AuditEnvelope> search(std::uint32_t session_id, std::string_view action = {}) const;
    std::string render(std::uint32_t session_id) const;
    std::size_t size() const;
    void clear();
private:
    std::uint64_t next_id_{1};
    std::vector<AuditEnvelope> events_;
};

struct ReplicaRecord {
    std::uint32_t session_id{};
    std::uint64_t generation{};
    std::string shard;
    std::string state;
    std::string digest;
};

class StateReplica {
public:
    void write(std::uint32_t session_id, std::uint64_t generation, std::string shard, std::string state);
    std::optional<ReplicaRecord> read(std::uint32_t session_id, std::string_view shard) const;
    std::vector<ReplicaRecord> replicas(std::uint32_t session_id) const;
    std::vector<std::string> validate(std::uint32_t session_id, std::uint64_t expected_generation) const;
    std::string report() const;
    void clear();
private:
    std::unordered_map<std::string, ReplicaRecord> records_;
};

struct CaptureObservation {
    std::uint64_t ordinal{};
    std::uint32_t session_id{};
    Operation operation{Operation::Heartbeat};
    std::size_t fields{};
    std::size_t bytes{};
    std::uint32_t checksum{};
};

class CaptureAnalyzer {
public:
    void observe(std::uint64_t ordinal, const Frame& frame, std::size_t bytes, std::uint32_t checksum);
    std::vector<CaptureObservation> session(std::uint32_t session_id) const;
    std::unordered_map<std::string, std::size_t> operation_counts() const;
    std::string report() const;
    void clear();
private:
    std::vector<CaptureObservation> observations_;
};

struct Alert {
    std::uint64_t id{};
    std::string source;
    std::string severity;
    std::string message;
    bool acknowledged{};
};

class AlertRouter {
public:
    std::uint64_t raise(std::string source, std::string severity, std::string message);
    bool acknowledge(std::uint64_t id);
    std::vector<Alert> outstanding() const;
    std::vector<Alert> by_severity(std::string_view severity) const;
    std::string report() const;
    void clear();
private:
    std::uint64_t next_id_{1};
    std::vector<Alert> alerts_;
};

struct PolicyDifference {
    std::string field;
    std::string before;
    std::string after;
    std::string impact;
};

class PolicyDiffer {
public:
    std::vector<PolicyDifference> compare(const PolicyCandidate& before, const PolicyCandidate& after) const;
    std::string summarize(const std::vector<PolicyDifference>& differences) const;
};

class PolicyCodec {
public:
    std::string encode(const PolicyCandidate& candidate) const;
    std::optional<PolicyCandidate> decode(std::string_view text) const;
    std::string fingerprint(const PolicyCandidate& candidate) const;
};

enum class SessionPhase : Byte { New = 0, Registered = 1, Staged = 2, Active = 3, Moving = 4, Closed = 5 };

struct StateTransition {
    SessionPhase from{SessionPhase::New};
    SessionPhase to{SessionPhase::New};
    Operation operation{Operation::Heartbeat};
    bool allowed{};
    std::string reason;
};

class SessionStateMachine {
public:
    SessionPhase infer(const Session& session, const CompiledPolicy* policy) const;
    StateTransition transition(SessionPhase current, Operation operation) const;
    std::vector<StateTransition> validate(const std::vector<Operation>& operations) const;
    std::string name(SessionPhase phase) const;
};

struct IdentityLease {
    std::uint32_t id{};
    std::string namespace_name;
    std::uint64_t generation{};
    std::uint64_t expires{};
    bool released{};
};

class IdentityAllocator {
public:
    std::optional<IdentityLease> allocate(std::string namespace_name, std::uint64_t generation,
                                          std::uint64_t ttl);
    bool release(std::uint32_t id);
    std::optional<IdentityLease> lookup(std::uint32_t id) const;
    std::size_t expire(std::uint64_t advance);
    std::string report() const;
    void clear();
private:
    std::uint32_t next_id_{1000};
    std::uint64_t tick_{};
    std::unordered_map<std::uint32_t, IdentityLease> leases_;
};

struct BearerChange {
    std::uint32_t bearer_id{};
    std::uint32_t qos{};
    bool remove{};
    std::string label;
};

class BearerTransaction {
public:
    void stage(BearerChange change);
    std::vector<std::string> validate(const BearerStore& store) const;
    bool commit(BearerStore& store, std::uint64_t generation);
    void rollback();
    std::size_t pending() const;
private:
    std::vector<BearerChange> changes_;
};

struct TunnelLease {
    std::uint32_t tunnel_id{};
    std::uint32_t peer{};
    std::uint64_t generation{};
    std::string endpoint;
};

class TunnelAllocator {
public:
    std::optional<TunnelLease> allocate(std::uint32_t peer, std::uint64_t generation,
                                        std::string endpoint);
    bool release(std::uint32_t tunnel_id);
    std::vector<TunnelLease> active() const;
    std::string report() const;
    void clear();
private:
    std::uint32_t next_id_{1};
    std::unordered_map<std::uint32_t, TunnelLease> leases_;
};

struct CachedRoute {
    std::uint32_t session_id{};
    std::uint64_t generation{};
    std::uint32_t route_id{};
    std::string key;
    std::uint64_t hits{};
};

class StableRouteCache {
public:
    void put(std::uint32_t session_id, std::uint64_t generation, std::uint32_t route_id, std::string key);
    std::optional<CachedRoute> get(std::uint32_t session_id, std::uint64_t generation) const;
    std::size_t invalidate_generation(std::uint32_t session_id, std::uint64_t keep_generation);
    std::vector<CachedRoute> entries() const;
    std::string report() const;
    void clear();
private:
    std::unordered_map<std::uint32_t, CachedRoute> entries_;
};

struct QuotaProjection {
    std::uint32_t session_id{};
    std::uint64_t observed{};
    std::uint64_t forecast{};
    std::uint64_t ceiling{};
    bool exceeds{};
};

class QuotaForecaster {
public:
    void observe(std::uint32_t session_id, std::uint64_t units);
    QuotaProjection project(std::uint32_t session_id, std::uint64_t ceiling) const;
    std::string report() const;
    void clear();
private:
    std::unordered_map<std::uint32_t, std::vector<std::uint64_t>> history_;
};

struct ReconciliationItem {
    std::uint32_t session_id{};
    std::uint64_t expected_units{};
    std::uint64_t charged_units{};
    std::int64_t delta{};
    std::vector<std::uint64_t> related_lines;
};

class ChargeReconciler {
public:
    ReconciliationItem reconcile(std::uint32_t session_id, const UsageAggregator& usage,
                                 const ChargingLedger& ledger) const;
    std::string explain(const ReconciliationItem& item) const;
};

struct FairnessSlot {
    std::uint32_t session_id{};
    std::uint64_t debt{};
    std::uint64_t served{};
    std::uint32_t priority{};
};

class FairScheduler {
public:
    void register_session(std::uint32_t session_id, std::uint32_t priority);
    void charge(std::uint32_t session_id, std::uint64_t units);
    std::optional<std::uint32_t> next() const;
    std::vector<FairnessSlot> slots() const;
    std::string report() const;
    void clear();
private:
    std::unordered_map<std::uint32_t, FairnessSlot> slots_;
};

class JournalCodec {
public:
    Bytes encode(const std::vector<JournalEntry>& entries) const;
    std::vector<JournalEntry> decode(std::span<const Byte> bytes, std::vector<std::string>& errors) const;
    std::string digest(const std::vector<JournalEntry>& entries) const;
};

class CheckpointCodec {
public:
    Bytes encode(const std::vector<Checkpoint>& points) const;
    std::vector<Checkpoint> decode(std::span<const Byte> bytes, std::vector<std::string>& errors) const;
    std::string digest(const Checkpoint& point) const;
};

struct RecoveryStep {
    std::string component;
    std::string action;
    std::uint64_t sequence{};
    std::vector<std::string> dependencies;
};

class RecoveryPlanner {
public:
    std::vector<RecoveryStep> plan(const std::vector<JournalEntry>& entries) const;
    std::vector<std::string> validate(const std::vector<RecoveryStep>& steps) const;
    std::string render(const std::vector<RecoveryStep>& steps) const;
};

struct TraceSpan {
    std::uint64_t id{};
    std::uint64_t parent{};
    std::string name;
    std::uint64_t start{};
    std::uint64_t end{};
    std::unordered_map<std::string, std::string> attributes;
};

class TraceBuffer {
public:
    std::uint64_t begin(std::string name, std::uint64_t parent = 0U);
    bool annotate(std::uint64_t id, std::string key, std::string value);
    bool end(std::uint64_t id);
    std::vector<TraceSpan> complete() const;
    std::string render() const;
    void clear();
private:
    std::uint64_t tick_{};
    std::uint64_t next_id_{1};
    std::unordered_map<std::uint64_t, TraceSpan> spans_;
};

struct DiagnosticRule {
    std::string key;
    std::int64_t threshold{};
    std::string severity;
    std::string message;
};

class DiagnosticCatalog {
public:
    DiagnosticCatalog();
    std::vector<Alert> evaluate(std::string_view key, std::int64_t value) const;
    std::vector<DiagnosticRule> rules() const;

private:
    std::vector<DiagnosticRule> rules_;
};

class FrameNormalizer {
public:
    Frame normalize(const Frame& frame) const;
    std::vector<std::string> validate(const Frame& frame) const;
    std::string describe(const Frame& frame) const;
};

struct CaptureExportRow {
    std::uint64_t ordinal{};
    std::uint32_t session_id{};
    std::string operation;
    std::uint64_t generation{};
    std::string summary;
};

class CaptureExporter {
public:
    std::vector<CaptureExportRow> rows(const std::vector<CaptureObservation>& observations,
                                       const SessionStore& sessions) const;
    std::string csv(const std::vector<CaptureExportRow>& rows) const;
};

class TopologyValidator {
public:
    std::vector<std::string> validate(const TopologyGraph& graph) const;
    std::vector<std::uint32_t> roots(const TopologyGraph& graph) const;
    std::string render(const TopologyGraph& graph) const;
};

class RouteAuditor {
public:
    std::vector<std::string> audit(const Session& session, const CompiledPolicy* policy,
                                   const StableRouteCache& cache) const;
    std::string render(const std::vector<std::string>& findings) const;
};

struct TrafficDecision {
    bool allowed{};
    std::uint64_t delay{};
    std::uint64_t allowance{};
    std::string reason;
};

class TrafficShaper {
public:
    void configure(std::uint32_t session_id, std::uint64_t allowance, std::uint64_t refill);
    TrafficDecision admit(std::uint32_t session_id, std::uint64_t units, std::uint64_t tick);
    std::string report() const;
    void clear();
private:
    struct State { std::uint64_t allowance{}; std::uint64_t max{}; std::uint64_t refill{}; std::uint64_t tick{}; };
    std::unordered_map<std::uint32_t, State> states_;
};
