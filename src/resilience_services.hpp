#pragma once

struct LoadDecision { bool allowed{}; std::uint32_t priority{}; std::string reason; };
class LoadShedder {
public:
    void configure(std::uint64_t threshold, std::uint32_t minimum_priority);
    LoadDecision decide(std::uint64_t observed, std::uint32_t priority) const;
    void clear();
private:
    std::uint64_t threshold_{1000U};
    std::uint32_t minimum_priority_{1U};
};

class ReplicationLagMonitor {
public:
    void observe(std::string replica, std::uint64_t produced, std::uint64_t applied);
    std::optional<std::uint64_t> lag(std::string_view replica) const;
    std::vector<std::string> behind(std::uint64_t limit) const;
    void clear();
private:
    std::unordered_map<std::string, std::uint64_t> lags_;
};

class JournalCompactor {
public:
    std::vector<JournalEntry> compact(const std::vector<JournalEntry>& entries, std::size_t limit) const;
    std::string digest(const std::vector<JournalEntry>& entries) const;
};

class SnapshotVerifier {
public:
    bool verify(const Checkpoint& checkpoint) const;
    std::vector<std::uint64_t> verify_all(const std::vector<Checkpoint>& checkpoints) const;
};

struct PolicyConflict { std::uint32_t session_id{}; std::string reason; };
class PolicyConflictMatrix {
public:
    void note(const PolicyCandidate& candidate);
    std::vector<PolicyConflict> conflicts() const;
    void clear();
private:
    std::unordered_map<std::uint32_t, PolicyCandidate> candidates_;
};

class FrameCoalescer {
public:
    std::vector<Frame> coalesce(const std::vector<Frame>& frames) const;
    std::size_t count_payload(const std::vector<Frame>& frames) const;
};

class SessionGroupIndex {
public:
    void put(std::uint32_t session_id, std::string group);
    bool erase(std::uint32_t session_id);
    std::vector<std::uint32_t> find(std::string_view group) const;
    void clear();
private:
    std::unordered_map<std::uint32_t, std::string> groups_;
};

class SliceBudgetBook {
public:
    void configure(std::string slice, std::uint64_t budget);
    bool consume(std::string_view slice, std::uint64_t units);
    std::uint64_t remaining(std::string_view slice) const;
    void clear();
private:
    std::unordered_map<std::string, std::uint64_t> budgets_;
};

class AccessPatternProfiler {
public:
    void observe(std::uint32_t session_id, Operation operation);
    std::vector<std::pair<std::string, std::uint64_t>> summary(std::uint32_t session_id) const;
    void clear();
private:
    std::unordered_map<std::uint32_t, std::map<std::string, std::uint64_t>> patterns_;
};

class RouteFailoverManager {
public:
    void install(std::uint32_t session_id, std::vector<Route> routes);
    std::optional<Route> select(std::uint32_t session_id, const std::set<std::string>& unavailable) const;
    void clear();
private:
    std::unordered_map<std::uint32_t, std::vector<Route>> routes_;
};

struct ExtensionSchema { std::uint8_t tag{}; std::size_t minimum{}; std::size_t maximum{}; std::string name; };
class ProtocolExtensionRegistry {
public:
    void register_schema(ExtensionSchema schema);
    bool accepts(std::uint8_t tag, std::size_t length) const;
    std::vector<ExtensionSchema> schemas() const;
    void clear();
private:
    std::map<std::uint8_t, ExtensionSchema> schemas_;
};

struct PipelineStage { std::uint64_t id{}; std::string name; bool completed{}; };
class CommandPipeline {
public:
    std::uint64_t enqueue(std::string name);
    bool complete(std::uint64_t id);
    std::vector<PipelineStage> pending() const;
    void clear();
private:
    std::uint64_t next_id_{1U};
    std::vector<PipelineStage> stages_;
};

class KeyRotationLedger {
public:
    void rotate(std::string domain, std::string fingerprint, std::uint64_t epoch);
    std::optional<std::string> active(std::string_view domain) const;
    std::vector<std::string> expiring(std::uint64_t before_epoch) const;
    void clear();
private:
    struct Key { std::string fingerprint; std::uint64_t epoch{}; };
    std::unordered_map<std::string, Key> keys_;
};

class ServiceQuotaAllocator {
public:
    void assign(std::string service, std::uint64_t capacity);
    std::uint64_t grant(std::string_view service, std::uint64_t requested);
    std::string report() const;
    void clear();
private:
    std::unordered_map<std::string, std::uint64_t> available_;
};

class TelemetryNormalizer {
public:
    std::map<std::string, std::uint64_t> normalize(const std::map<std::string, std::uint64_t>& raw) const;
    std::string render(const std::map<std::string, std::uint64_t>& values) const;
};

class PredictiveScheduler {
public:
    void observe(std::string key, std::uint64_t tick);
    std::optional<std::uint64_t> next(std::string_view key) const;
    void clear();
private:
    std::unordered_map<std::string, std::vector<std::uint64_t>> observations_;
};

class IncidentTimeline {
public:
    void append(std::uint64_t tick, std::string severity, std::string message);
    std::vector<std::string> since(std::uint64_t tick) const;
    void clear();
private:
    struct Item { std::uint64_t tick{}; std::string severity; std::string message; };
    std::vector<Item> items_;
};

class AuditSigner {
public:
    std::string sign(std::string_view value, std::string_view domain) const;
    bool verify(std::string_view value, std::string_view domain, std::string_view signature) const;
};

class TrafficPolicyMatrix {
public:
    void set(std::string source, std::string destination, bool allowed);
    bool allowed(std::string_view source, std::string_view destination) const;
    void clear();
private:
    std::map<std::pair<std::string, std::string>, bool> rules_;
};

class ResilienceServiceSuite {
public:
    ResilienceServiceSuite();
    void observe(const Frame& frame, bool accepted, std::uint64_t tick);
    std::string report() const;
    void clear();
private:
    LoadShedder shedder_;
    ReplicationLagMonitor replication_;
    JournalCompactor compactor_;
    SnapshotVerifier snapshots_;
    PolicyConflictMatrix conflicts_;
    FrameCoalescer coalescer_;
    SessionGroupIndex groups_;
    SliceBudgetBook budgets_;
    AccessPatternProfiler patterns_;
    RouteFailoverManager failover_;
    ProtocolExtensionRegistry extensions_;
    CommandPipeline pipeline_;
    KeyRotationLedger keys_;
    ServiceQuotaAllocator quotas_;
    TelemetryNormalizer telemetry_;
    PredictiveScheduler schedule_;
    IncidentTimeline incidents_;
    AuditSigner signer_;
    TrafficPolicyMatrix traffic_;
    ScenarioControlService scenarios_;
    std::vector<Frame> recent_;
};
