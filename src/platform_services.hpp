#pragma once

struct ArchiveEntry {
    std::uint64_t ordinal{};
    std::uint32_t session_id{};
    Operation operation{Operation::Heartbeat};
    std::uint32_t checksum{};
    Bytes encoded;
};

class CaptureArchive {
public:
    void append(const PacketRecord& record);
    std::vector<ArchiveEntry> session(std::uint32_t session_id) const;
    std::vector<ArchiveEntry> range(std::uint64_t first, std::uint64_t last) const;
    Bytes encode() const;
    std::vector<ArchiveEntry> decode(std::span<const Byte> bytes, std::vector<std::string>& errors) const;
    std::size_t compact(std::size_t keep_per_session);
    std::string report() const;
    void clear();
private:
    std::vector<ArchiveEntry> entries_;
};

struct PolicyTextDocument {
    std::map<std::string, std::string> values;
    std::vector<std::string> warnings;
};

class PolicyTextParser {
public:
    PolicyTextDocument parse(std::string_view text) const;
    std::vector<std::string> validate(const PolicyTextDocument& document) const;
    std::optional<PolicyCandidate> candidate(std::uint32_t session_id, const PolicyTextDocument& document) const;
    std::string render(const PolicyTextDocument& document) const;
private:
    std::optional<std::uint32_t> number(std::string_view text) const;
    std::string trim(std::string_view text) const;
};

struct RecoveryAction {
    std::uint64_t sequence{};
    std::uint32_t session_id{};
    std::uint64_t generation{};
    std::string component;
    std::string action;
    std::string payload;
    std::vector<std::uint64_t> prerequisites;
};

class RecoveryOrchestrator {
public:
    std::vector<RecoveryAction> plan(const std::vector<RecoveryEntry>& entries,
                                     const std::vector<Checkpoint>& checkpoints) const;
    std::vector<std::string> validate(const std::vector<RecoveryAction>& actions) const;
    std::vector<RecoveryAction> ready(const std::vector<RecoveryAction>& actions,
                                      const std::set<std::uint64_t>& completed) const;
    std::string render(const std::vector<RecoveryAction>& actions) const;
};

struct DiagnosticObservation {
    std::uint64_t ordinal{};
    std::uint32_t session_id{};
    std::string category;
    std::string severity;
    std::string detail;
    std::uint64_t value{};
};

class DiagnosticPipeline {
public:
    void observe(std::uint64_t ordinal, const Frame& frame, std::span<const Byte> encoded);
    std::vector<DiagnosticObservation> session(std::uint32_t session_id) const;
    std::vector<DiagnosticObservation> severity(std::string_view value) const;
    std::map<std::string, std::uint64_t> totals() const;
    std::string render(std::size_t limit = 32U) const;
    void clear();
private:
    void add(DiagnosticObservation observation);
    std::vector<DiagnosticObservation> observations_;
};

struct VersionedSnapshot {
    std::uint64_t id{};
    std::uint32_t session_id{};
    std::uint64_t generation{};
    std::uint32_t version{};
    std::string kind;
    Bytes payload;
    std::string digest;
};

class VersionedSnapshotStore {
public:
    std::uint64_t capture(std::uint32_t session_id, std::uint64_t generation, std::string kind,
                          std::string_view state, std::uint32_t version = 1U);
    std::optional<VersionedSnapshot> latest(std::uint32_t session_id, std::string_view kind = {}) const;
    std::vector<VersionedSnapshot> history(std::uint32_t session_id) const;
    bool verify(const VersionedSnapshot& snapshot) const;
    std::size_t prune(std::uint32_t session_id, std::size_t keep);
    std::string report() const;
    void clear();
private:
    std::uint64_t next_id_{1U};
    std::unordered_map<std::uint32_t, std::vector<VersionedSnapshot>> snapshots_;
};

struct ServiceTopologyNode {
    std::string name;
    std::uint64_t capacity{};
    std::uint64_t allocated{};
    std::set<std::string> neighbors;
};

class ServiceTopologyModel {
public:
    void upsert(std::string name, std::uint64_t capacity);
    void connect(std::string left, std::string right);
    bool allocate(std::string_view name, std::uint64_t amount);
    void release(std::string_view name, std::uint64_t amount);
    std::vector<std::string> path(std::string_view source, std::string_view destination) const;
    std::string report() const;
    void clear();
private:
    std::unordered_map<std::string, ServiceTopologyNode> nodes_;
};
