#include "internal.hpp"

namespace aegis::detail {

namespace {
Bytes u32_bytes(std::uint32_t value) {
    return {static_cast<Byte>((value >> 24U) & 0xffU), static_cast<Byte>((value >> 16U) & 0xffU),
            static_cast<Byte>((value >> 8U) & 0xffU), static_cast<Byte>(value & 0xffU)};
}

Field field(FieldTag tag, Bytes value) { return {static_cast<Byte>(tag), std::move(value)}; }
Field text_field(FieldTag tag, std::string value) { return {static_cast<Byte>(tag), Bytes(value.begin(), value.end())}; }
}

SyntheticTrafficGenerator::SyntheticTrafficGenerator(std::uint32_t seed)
    : state_(static_cast<std::uint64_t>(seed == 0U ? 0x9e3779b9U : seed) | 1U) {}

std::uint32_t SyntheticTrafficGenerator::next_u32() {
    state_ ^= state_ << 13U;
    state_ ^= state_ >> 7U;
    state_ ^= state_ << 17U;
    return static_cast<std::uint32_t>((state_ >> 16U) ^ state_);
}

Bytes SyntheticTrafficGenerator::payload(std::uint32_t minimum, std::uint32_t maximum) {
    const auto low = std::min(minimum, maximum);
    const auto high = std::max(minimum, maximum);
    const auto length = low + (high == low ? 0U : next_u32() % (high - low + 1U));
    Bytes bytes;
    bytes.reserve(length);
    for (std::uint32_t index = 0; index < length; ++index) {
        const auto value = next_u32();
        bytes.push_back(static_cast<Byte>(32U + (value % 91U)));
    }
    return bytes;
}

SimulationRun SyntheticTrafficGenerator::generate(const SimulationParameters& parameters) {
    SimulationRun run;
    const auto session = parameters.session_id;
    const auto route = (next_u32() % 4095U) + 1U;
    const auto profile = (next_u32() % 65535U) + 1U;
    const auto policy_version = (next_u32() % 32767U) + 1U;
    std::uint32_t sequence = 1U;
    auto append = [&](Operation operation, std::vector<Field> fields, std::string category, std::uint64_t units) {
        run.frames.push_back({operation, 0U, session, std::move(fields)});
        run.metrics.push_back({run.frames.size(), std::move(category), units, describe_operation(operation)});
        run.generated_payload += units;
    };
    append(Operation::Open, {field(FieldTag::ProfileId, u32_bytes(profile)), field(FieldTag::RouteId, u32_bytes(route)), text_field(FieldTag::NextHop, "sim-primary"), field(FieldTag::Sequence, u32_bytes(sequence++)), field(FieldTag::Quota, u32_bytes(4096U))}, "control", 1U);
    append(Operation::Register, {text_field(FieldTag::Subscriber, "sim-" + std::to_string(session)), field(FieldTag::Sequence, u32_bytes(sequence++))}, "control", 1U);
    append(Operation::StagePolicy, {field(FieldTag::ProfileId, u32_bytes(profile + 1U)), field(FieldTag::RouteId, u32_bytes(route)), field(FieldTag::PolicyVersion, u32_bytes(policy_version)), text_field(FieldTag::NextHop, "sim-primary"), text_field(FieldTag::Label, "standard"), field(FieldTag::Sequence, u32_bytes(sequence++))}, "policy", 2U);
    append(Operation::Commit, {field(FieldTag::PolicyVersion, u32_bytes(policy_version)), field(FieldTag::Sequence, u32_bytes(sequence++))}, "policy", 2U);
    const auto rounds = std::max(1U, parameters.rounds);
    for (std::uint32_t round = 0; round < rounds; ++round) {
        const auto bytes = payload(parameters.minimum_payload, parameters.maximum_payload);
        const auto size = bytes.size();
        append(Operation::Data, {field(FieldTag::Payload, bytes), field(FieldTag::Sequence, u32_bytes(sequence++)), text_field(FieldTag::Label, round % 2U == 0U ? "steady" : "burst")}, "data", size);
        if (round != 0U && round % 4U == 0U) append(Operation::Heartbeat, {field(FieldTag::Sequence, u32_bytes(sequence++))}, "control", 1U);
    }
    append(Operation::Handover, {field(FieldTag::HandoverTarget, u32_bytes((next_u32() % 31U) + 1U)), field(FieldTag::Sequence, u32_bytes(sequence++))}, "handover", 3U);
    append(Operation::Audit, {field(FieldTag::Sequence, u32_bytes(sequence++))}, "audit", 2U);
    append(Operation::Snapshot, {field(FieldTag::Sequence, u32_bytes(sequence++))}, "snapshot", 2U);
    append(Operation::Close, {field(FieldTag::Sequence, u32_bytes(sequence++))}, "control", 1U);
    return run;
}

void SimulationLedger::add(SimulationMetric metric) { metrics_.push_back(std::move(metric)); }

std::uint64_t SimulationLedger::total(std::string_view category) const {
    std::uint64_t total{};
    for (const auto& metric : metrics_) if (metric.category == category) total += metric.units;
    return total;
}

std::vector<SimulationMetric> SimulationLedger::by_category(std::string_view category) const {
    std::vector<SimulationMetric> output;
    for (const auto& metric : metrics_) if (metric.category == category) output.push_back(metric);
    return output;
}

std::map<std::string, std::uint64_t> SimulationLedger::grouped() const {
    std::map<std::string, std::uint64_t> output;
    for (const auto& metric : metrics_) output[metric.category] += metric.units;
    return output;
}

void SimulationLedger::clear() { metrics_.clear(); }

std::string SimulationRenderer::render_frames(const std::vector<Frame>& frames) const {
    std::vector<std::vector<std::string>> rows{{"ordinal", "operation", "session", "fields", "encoded"}};
    for (std::size_t index = 0; index < frames.size(); ++index) {
        const auto bytes = write_frame(frames[index]);
        rows.push_back({std::to_string(index + 1U), describe_operation(frames[index].operation), std::to_string(frames[index].session_id), std::to_string(frames[index].fields.size()), std::to_string(bytes.size())});
    }
    return render_table(rows);
}

std::string SimulationRenderer::render(const SimulationRun& run, const IngestResult& result,
                                       const SimulationLedger& ledger, std::string_view engine_inspection) const {
    std::vector<std::vector<std::string>> summary{{"metric", "value"}};
    summary.push_back({"frames", std::to_string(run.frames.size())});
    summary.push_back({"generated_payload", std::to_string(run.generated_payload)});
    summary.push_back({"accepted", std::to_string(result.accepted_frames)});
    summary.push_back({"rejected", std::to_string(result.rejected_frames)});
    for (const auto& [category, total] : ledger.grouped()) summary.push_back({"units." + category, std::to_string(total)});
    std::string output = render_table(summary) + "\n" + render_frames(run.frames);
    if (!result.diagnostics.empty()) output += "\nDiagnostics\n" + join_labels(result.diagnostics, "\n");
    if (!engine_inspection.empty()) output += "\nInspection\n" + std::string(engine_inspection);
    return output;
}

SimulationParameters ControlPlaneSimulator::normalize(SimulationParameters parameters) const {
    parameters.session_id = parameters.session_id == 0U ? 1U : parameters.session_id;
    parameters.rounds = std::clamp(parameters.rounds, 1U, 512U);
    parameters.seed = parameters.seed == 0U ? 1U : parameters.seed;
    parameters.minimum_payload = std::clamp(parameters.minimum_payload, 1U, 4096U);
    parameters.maximum_payload = std::clamp(parameters.maximum_payload, parameters.minimum_payload, 8192U);
    return parameters;
}

SimulationRun ControlPlaneSimulator::generate(const SimulationParameters& parameters) const {
    const auto normalized = normalize(parameters);
    SyntheticTrafficGenerator generator(normalized.seed);
    return generator.generate(normalized);
}

Bytes ControlPlaneSimulator::capture(const SimulationParameters& parameters) const {
    const auto run = generate(parameters);
    return write_capture(run.frames);
}

std::string ControlPlaneSimulator::run(const SimulationParameters& parameters) const {
    const auto generated = generate(parameters);
    const auto repeated = generate(parameters);
    SimulationComparator comparator;
    const auto deterministic = comparator.equivalent(generated, repeated);
    SimulationValidator validator;
    const auto validation = validator.validate(generated, normalize(parameters));
    SimulationSeedCatalog seed_catalog;
    const auto curated = seed_catalog.build(normalize(parameters));
    const auto seed_errors = seed_catalog.validate(curated);
    SimulationLedger ledger;
    for (const auto& metric : generated.metrics) ledger.add(metric);
    EngineCore core;
    const auto data = write_capture(generated.frames);
    const auto result = core.ingest(data);
    SimulationRenderer renderer;
    auto report = renderer.render(generated, result, ledger, core.inspect());
    report += "\nDeterministic: ";
    report += deterministic ? "yes" : "no";
    report += "\nSimulation validation\n" + validator.render(validation);
    report += "\nSeed catalog\n" + seed_catalog.manifest(curated);
    if (!seed_errors.empty()) report += "\nSeed errors\n" + join_labels(seed_errors, "\n");
    if (!deterministic) report += "\n" + comparator.render(comparator.compare(generated, repeated));
    return report;
}

}  // namespace aegis::detail
