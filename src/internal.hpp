#pragma once

#include "aegis/aegis.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <functional>
#include <iomanip>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <tuple>
#include <variant>
#include <vector>

namespace aegis::detail {

using Byte = std::uint8_t;
using Bytes = std::vector<Byte>;

constexpr std::array<Byte, 4> kFrameMagic{{'A', 'G', 'F', '1'}};
constexpr std::size_t kFrameHeaderBytes = 12;
constexpr std::uint32_t kIndexSentinel = 0xfffffffaU;

enum class Operation : Byte {
    Open = 1,
    Register = 2,
    StagePolicy = 3,
    Commit = 4,
    Data = 5,
    Handover = 6,
    Rollback = 7,
    Close = 8,
    Topology = 9,
    Audit = 10,
    Snapshot = 11,
    Recover = 12,
    Extensions = 13,
    Heartbeat = 14,
};

enum class FieldTag : Byte {
    ProfileId = 1,
    RouteId = 2,
    NextHop = 3,
    PolicyVersion = 4,
    Extension = 5,
    Sequence = 6,
    Shard = 7,
    Topology = 8,
    Snapshot = 9,
    MirrorAudit = 10,
    RecoveryCount = 11,
    RouteMode = 12,
    Label = 13,
    Subscriber = 14,
    Quota = 15,
    Payload = 16,
    HandoverTarget = 17,
};

struct DecodeError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct Cursor {
    std::span<const Byte> input;
    std::size_t offset{};
    explicit Cursor(std::span<const Byte> bytes) : input(bytes) {}
    std::size_t remaining() const;
    bool empty() const;
    Byte u8();
    std::uint16_t u16();
    std::uint32_t u32();
    std::uint64_t u64();
    Bytes bytes(std::size_t count);
    std::string text(std::size_t count);
    void skip(std::size_t count);
};

struct Writer {
    Bytes output;
    void u8(Byte value);
    void u16(std::uint16_t value);
    void u32(std::uint32_t value);
    void u64(std::uint64_t value);
    void bytes(std::span<const Byte> value);
    void text(std::string_view value);
    Bytes take();
};

std::uint32_t crc32(std::span<const Byte> data);
std::uint32_t checksum32(std::span<const Byte> data);
Bytes encode_varint(std::uint64_t value);
std::uint64_t decode_varint(Cursor& cursor);
std::string hex(std::span<const Byte> data);
Bytes unhex(std::string_view text);
bool valid_utf8(std::span<const Byte> data);
std::string escaped_json(std::string_view value);
std::string canonical_text(std::string_view value);
std::vector<std::string> split_labels(std::string_view value);
std::string join_labels(const std::vector<std::string>& labels, std::string_view separator);

struct Field {
    Byte tag{};
    Bytes value;
};

struct Frame {
    Operation operation{Operation::Heartbeat};
    Byte flags{};
    std::uint32_t session_id{};
    std::vector<Field> fields;
};

Frame parse_frame(Cursor& cursor);
Bytes write_frame(const Frame& frame);
std::vector<Frame> parse_capture(std::span<const Byte> data, std::vector<std::string>& errors);
Bytes write_capture(const std::vector<Frame>& frames);
const Field* find_field(const Frame& frame, FieldTag tag);
std::uint32_t field_u32(const Frame& frame, FieldTag tag, std::uint32_t fallback = 0);
std::string field_text(const Frame& frame, FieldTag tag, std::string fallback = {});
Bytes field_bytes(const Frame& frame, FieldTag tag);
bool field_flag(const Frame& frame, FieldTag tag);

struct PacketRecord {
    std::uint64_t ordinal{};
    Frame frame;
    std::uint32_t checksum{};
};

struct PacketBatch {
    std::vector<PacketRecord> records;
    std::uint64_t first_ordinal{};
    std::uint64_t last_ordinal{};
};

class CaptureIndex {
public:
    void add(const PacketRecord& record);
    std::vector<std::uint64_t> session_offsets(std::uint32_t session_id) const;
    std::size_t size() const;
    void clear();
private:
    std::unordered_map<std::uint32_t, std::vector<std::uint64_t>> offsets_;
};

class StreamWindow {
public:
    explicit StreamWindow(std::size_t limit = 64);
    void push(PacketRecord record);
    std::vector<PacketRecord> recent(std::size_t count) const;
    std::size_t size() const;
    void clear();
private:
    std::size_t limit_;
    std::deque<PacketRecord> records_;
};

class FragmentBuffer {
public:
    void append(std::uint32_t stream_id, std::span<const Byte> data);
    std::optional<Bytes> complete(std::uint32_t stream_id, std::size_t expected);
    void discard(std::uint32_t stream_id);
    std::size_t buffered() const;
private:
    std::unordered_map<std::uint32_t, Bytes> fragments_;
};

struct SubscriberId {
    std::uint32_t value{};
    std::string normalized;
};

struct SubscriberProfile {
    SubscriberId id;
    std::string display_name;
    std::uint64_t revision{};
    std::set<std::string> permissions;
};

class IdentityMap {
public:
    void put(SubscriberProfile profile);
    std::optional<SubscriberProfile> get(std::uint32_t id) const;
    void erase(std::uint32_t id);
    std::size_t size() const;
    std::vector<SubscriberProfile> all() const;
private:
    std::unordered_map<std::uint32_t, SubscriberProfile> profiles_;
};

struct Bearer {
    std::uint32_t id{};
    std::uint32_t qos{};
    std::uint64_t generation{};
    bool active{};
    std::string label;
};

class BearerStore {
public:
    void install(Bearer bearer);
    std::optional<Bearer> find(std::uint32_t id) const;
    void remove(std::uint32_t id);
    std::vector<Bearer> active() const;
    std::size_t size() const;
private:
    std::unordered_map<std::uint32_t, Bearer> bearers_;
};

struct Tunnel {
    std::uint32_t id{};
    std::uint32_t peer{};
    std::uint64_t generation{};
    bool bound{};
    std::string endpoint;
};

class TunnelStore {
public:
    void bind(Tunnel tunnel);
    std::optional<Tunnel> lookup(std::uint32_t id) const;
    void unbind(std::uint32_t id);
    std::vector<Tunnel> bound() const;
    std::size_t size() const;
private:
    std::unordered_map<std::uint32_t, Tunnel> tunnels_;
};

class ReplayGuard {
public:
    bool accept(std::uint32_t session_id, std::uint32_t sequence);
    void reset(std::uint32_t session_id);
    std::size_t rejected() const;
    std::vector<std::uint32_t> observed(std::uint32_t session_id) const;
private:
    struct Window { std::uint32_t high{}; std::uint64_t bitmap{}; bool initialized{}; };
    std::unordered_map<std::uint32_t, Window> windows_;
    std::size_t rejected_{};
};

struct Route {
    std::uint32_t id{};
    std::string next_hop;
    std::uint32_t weight{1};
    std::uint64_t profile_generation{};
    std::vector<std::string> labels;
    bool enabled{true};
};

struct RouteRule {
    std::uint32_t route_id{};
    std::uint32_t quota{};
    std::string match;
    std::uint64_t version{};
};

class RoutingTable {
public:
    void install(Route route);
    std::optional<Route> find(std::uint32_t id) const;
    std::vector<Route> select(std::string_view label) const;
    void erase(std::uint32_t id);
    std::size_t size() const;
    std::vector<Route> all() const;
private:
    std::unordered_map<std::uint32_t, Route> routes_;
};

class RouteIndex {
public:
    void rebuild(const std::vector<Route>& routes);
    std::vector<std::uint32_t> lookup(std::string_view label) const;
    void clear();
private:
    std::unordered_map<std::string, std::vector<std::uint32_t>> index_;
};

struct OpaqueExtension {
    std::vector<Byte> bytes;
    std::uint32_t marker{};
};

struct RouteHintsExtension {
    std::array<std::uint64_t, 8> guards{};
    std::vector<std::uint32_t> hops;
    std::string source;
    std::uint32_t lane{};
};

enum class ExtensionKind : Byte { Opaque = 0, RouteHints = 1, Identity = 2, Audit = 3 };

struct ExtensionSlot {
    ExtensionKind kind{ExtensionKind::Opaque};
    void* payload{};
    void (*destroy)(void*){};
    ExtensionSlot() = default;
    ExtensionSlot(ExtensionKind kind_value, void* data, void (*releaser)(void*))
        : kind(kind_value), payload(data), destroy(releaser) {}
    ExtensionSlot(const ExtensionSlot&) = delete;
    ExtensionSlot& operator=(const ExtensionSlot&) = delete;
    ExtensionSlot(ExtensionSlot&& other) noexcept;
    ExtensionSlot& operator=(ExtensionSlot&& other) noexcept;
    ~ExtensionSlot();
};

std::vector<ExtensionSlot> decode_extensions(std::span<const Byte> data);
Bytes encode_extensions(const std::vector<ExtensionSlot>& slots);
void canonicalize_extensions(std::vector<ExtensionSlot>& slots, bool route_mode);
std::uint64_t bind_extension_routes(const std::vector<ExtensionSlot>& slots);
std::string extension_summary(const std::vector<ExtensionSlot>& slots);

struct PolicyAttachment {
    std::string name;
    Bytes bytes;
    std::uint64_t checksum{};
    std::array<std::uint64_t, 4> accounting{};
};

struct CompiledPolicy {
    std::uint32_t session_id{};
    std::uint64_t version{};
    std::uint32_t route_id{};
    std::string route_label;
    bool route_mode{};
    std::vector<ExtensionSlot> extensions;
    std::unique_ptr<PolicyAttachment> attachment;
    std::vector<RouteRule> rules;
};

struct PolicyCandidate {
    std::uint32_t session_id{};
    std::uint32_t profile_id{};
    std::uint32_t route_id{};
    std::uint64_t version{};
    std::string next_hop;
    std::string label;
    Bytes extension;
    bool mirror_audit{};
    bool route_mode{};
};

struct AuditRecord {
    std::uint32_t session_id{};
    std::string action;
    std::uint64_t sequence{};
    std::string digest;
    std::unique_ptr<PolicyAttachment> attachment;
};

class AuditJournal {
public:
    void record(std::uint32_t session_id, std::string action, std::string digest);
    void record_attachment(std::uint32_t session_id, PolicyAttachment* attachment);
    void erase_session(std::uint32_t session_id);
    std::size_t size() const;
    std::vector<AuditRecord> summaries() const;
    std::string render() const;
private:
    std::uint64_t next_sequence_{1};
    std::vector<AuditRecord> records_;
};

class PolicyCompiler {
public:
    CompiledPolicy compile(const PolicyCandidate& candidate) const;
    std::vector<RouteRule> build_rules(const PolicyCandidate& candidate) const;
    std::string digest(const PolicyCandidate& candidate) const;
};

class PolicyRepository {
public:
    void stage(PolicyCandidate candidate);
    bool has_stage(std::uint32_t session_id) const;
    std::optional<PolicyCandidate> staged(std::uint32_t session_id) const;
    void commit(std::uint32_t session_id);
    void rollback(std::uint32_t session_id);
    const CompiledPolicy* active(std::uint32_t session_id) const;
    std::size_t active_count() const;
    std::size_t staged_count() const;
    AuditJournal& journal();
    const AuditJournal& journal() const;
    void clear();
private:
    PolicyCompiler compiler_;
    std::unordered_map<std::uint32_t, PolicyCandidate> staged_;
    std::unordered_map<std::uint32_t, std::unique_ptr<CompiledPolicy>> active_;
    AuditJournal journal_;
};

struct Profile {
    std::uint32_t id{};
    std::uint64_t generation{};
    std::vector<Route> routes;
    std::vector<std::string> attributes;
    std::string owner;
};

struct Session {
    std::uint32_t id{};
    SubscriberId subscriber;
    std::unique_ptr<Profile> profile;
    std::unique_ptr<Profile> staged_profile;
    BearerStore bearers;
    TunnelStore tunnels;
    std::uint64_t generation{};
    std::uint32_t accepted_frames{};
    std::uint32_t quota{};
    bool closed{};
};

class SessionStore {
public:
    Session& open(std::uint32_t session_id, std::uint32_t profile_id, std::string next_hop, std::uint32_t route_id);
    Session* find(std::uint32_t session_id);
    const Session* find(std::uint32_t session_id) const;
    void stage_profile(std::uint32_t session_id, std::uint32_t profile_id, std::string next_hop, std::uint32_t route_id);
    void commit_profile(std::uint32_t session_id);
    void close(std::uint32_t session_id);
    std::vector<Session*> all();
    std::vector<const Session*> all() const;
    std::size_t size() const;
    void clear();
private:
    std::unordered_map<std::uint32_t, std::unique_ptr<Session>> sessions_;
};

struct DispatchPlan {
    std::uint32_t session_id{};
    std::uint64_t generation{};
    const Route* selected{};
    std::vector<std::uint32_t> candidate_ids;
    std::string key;
    std::uint64_t extension_signature{};
};

class Dispatcher {
public:
    std::string dispatch(Session& session, const CompiledPolicy* policy);
    void invalidate(std::uint32_t session_id);
    void clear();
    std::size_t size() const;
private:
    DispatchPlan build_plan(Session& session, const CompiledPolicy* policy);
    std::string emit(const DispatchPlan& plan, const Session& session) const;
    std::unordered_map<std::uint32_t, DispatchPlan> plans_;
};

struct TopologyNode {
    std::uint32_t id{};
    std::vector<std::uint32_t> refs;
    std::string label;
};

struct TopologyGraph {
    std::vector<TopologyNode> nodes;
    std::vector<std::uint32_t> roots;
};

TopologyGraph decode_topology(std::span<const Byte> data);
Bytes encode_topology(const TopologyGraph& graph);

class TopologyInspector {
public:
    std::vector<std::uint32_t> walk(const TopologyGraph& graph) const;
    std::vector<std::vector<std::uint32_t>> components(const TopologyGraph& graph) const;
    std::string summarize(const TopologyGraph& graph) const;
};

struct RecoveryEntry {
    std::uint32_t session_id{};
    std::uint64_t generation{};
    std::string label;
    Bytes state;
};

struct MigrationTicket {
    RecoveryEntry* source{};
    std::uint32_t destination{};
    std::uint64_t captured_generation{};
};

class RecoveryManager {
public:
    void capture_handover(const Session& session, std::uint32_t destination);
    void restore_synthetic(std::uint32_t count, std::uint32_t seed);
    std::size_t apply_tickets();
    std::size_t size() const;
    std::size_t tickets() const;
    std::vector<RecoveryEntry> entries() const;
    Bytes snapshot() const;
    void restore_snapshot(std::span<const Byte> data);
    void clear();
private:
    std::vector<RecoveryEntry> entries_;
    std::vector<MigrationTicket> tickets_;
};

class Diagnostics {
public:
    std::string render_session(const Session& session, const CompiledPolicy* policy) const;
    std::string render_engine(const std::vector<const Session*>& sessions, const AuditJournal& audit) const;
    std::vector<std::string> validate_session(const Session& session) const;
};

class Metrics {
public:
    void count(std::string key, std::uint64_t amount = 1);
    void observe(std::string key, std::uint64_t value);
    std::uint64_t value(std::string_view key) const;
    std::string report() const;
    void clear();
private:
    std::unordered_map<std::string, std::uint64_t> counters_;
    std::unordered_map<std::string, std::vector<std::uint64_t>> samples_;
};

#include "services.hpp"
#include "advanced_services.hpp"
#include "scenario_services.hpp"
#include "simulation.hpp"
#include "simulation_seed_catalog.hpp"
#include "platform_services.hpp"
#include "resilience_services.hpp"
#include "operations.hpp"

class EngineCore {
public:
    IngestResult ingest(std::span<const Byte> data);
    EngineStats stats() const;
    std::string inspect() const;
    Bytes snapshot() const;
    IngestResult restore(std::span<const Byte> data);
    void reset();
private:
    void apply(const Frame& frame, IngestResult& result);
    void apply_open(const Frame& frame);
    void apply_register(const Frame& frame);
    void apply_stage(const Frame& frame);
    void apply_commit(const Frame& frame);
    void apply_data(const Frame& frame);
    void apply_handover(const Frame& frame);
    void apply_rollback(const Frame& frame);
    void apply_close(const Frame& frame);
    void apply_topology(const Frame& frame);
    void apply_audit(const Frame& frame);
    void apply_recover(const Frame& frame);
    void apply_heartbeat(const Frame& frame);

    SessionStore sessions_;
    PolicyRepository policies_;
    Dispatcher dispatcher_;
    IdentityMap identities_;
    ReplayGuard replay_;
    RecoveryManager recovery_;
    CaptureIndex capture_index_;
    StreamWindow window_;
    Metrics metrics_;
    Diagnostics diagnostics_;
    QuotaManager quota_;
    ChargingLedger charging_;
    PriorityScheduler scheduler_;
    MobilityTracker mobility_;
    EventJournal events_;
    CheckpointManager checkpoints_;
    HealthMonitor health_;
    StreamReassembler reassembler_;
    RuleEvaluator rules_;
    RetentionStore retention_;
    TransactionCoordinator transactions_;
    ConfigurationCatalog configuration_;
    UsageAggregator usage_;
    IngressFilter ingress_;
    RateLimiter rate_limiter_;
    CircuitBreaker breaker_;
    TenantRegistry tenants_;
    ErrorCatalog errors_;
    IntegrityVerifier integrity_;
    MaintenancePlanner maintenance_;
    StatisticsRegistry statistics_;
    SessionTimeline timeline_;
    SnapshotCatalog snapshot_catalog_;
    RoutePlanner route_planner_;
    AuditPipeline audit_pipeline_;
    StateReplica replicas_;
    CaptureAnalyzer capture_analyzer_;
    AlertRouter alerts_;
    CaptureArchive archive_;
    PolicyTextParser policy_text_;
    RecoveryOrchestrator recovery_orchestrator_;
    DiagnosticPipeline diagnostic_pipeline_;
    VersionedSnapshotStore versioned_snapshots_;
    ServiceTopologyModel service_topology_;
    OperationsFabric operations_;
    std::unordered_map<std::uint32_t, TopologyGraph> topologies_;
    std::uint64_t ordinal_{};
};

std::string describe_operation(Operation operation);
std::string stable_digest(std::string_view text);
std::string render_table(const std::vector<std::vector<std::string>>& rows);
std::vector<std::string> validate_policy_candidate(const PolicyCandidate& candidate);
std::string query_session(const Session& session);
std::string serialize_session(const Session& session);
std::optional<Session> deserialize_session(std::string_view text);

}  // namespace aegis::detail
