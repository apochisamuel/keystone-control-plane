#pragma once

struct SimulationParameters {
    std::uint32_t session_id{1U};
    std::uint32_t rounds{8U};
    std::uint32_t seed{1U};
    std::uint32_t minimum_payload{16U};
    std::uint32_t maximum_payload{96U};
};

struct SimulationMetric {
    std::uint64_t ordinal{};
    std::string category;
    std::uint64_t units{};
    std::string detail;
};

struct SimulationRun {
    std::vector<Frame> frames;
    std::vector<SimulationMetric> metrics;
    std::uint64_t generated_payload{};
};

class SyntheticTrafficGenerator {
public:
    explicit SyntheticTrafficGenerator(std::uint32_t seed);
    std::uint32_t next_u32();
    Bytes payload(std::uint32_t minimum, std::uint32_t maximum);
    SimulationRun generate(const SimulationParameters& parameters);
private:
    std::uint64_t state_{};
};

class SimulationLedger {
public:
    void add(SimulationMetric metric);
    std::uint64_t total(std::string_view category) const;
    std::vector<SimulationMetric> by_category(std::string_view category) const;
    std::map<std::string, std::uint64_t> grouped() const;
    void clear();
private:
    std::vector<SimulationMetric> metrics_;
};

class SimulationRenderer {
public:
    std::string render(const SimulationRun& run, const IngestResult& result,
                       const SimulationLedger& ledger, std::string_view engine_inspection) const;
    std::string render_frames(const std::vector<Frame>& frames) const;
};

class ControlPlaneSimulator {
public:
    Bytes capture(const SimulationParameters& parameters) const;
    std::string run(const SimulationParameters& parameters) const;
    SimulationParameters normalize(SimulationParameters parameters) const;
private:
    SimulationRun generate(const SimulationParameters& parameters) const;
};

struct SimulationDelta {
    std::string category;
    std::int64_t frame_delta{};
    std::int64_t unit_delta{};
    std::string explanation;
};

class SimulationComparator {
public:
    std::vector<SimulationDelta> compare(const SimulationRun& baseline, const SimulationRun& candidate) const;
    std::string render(const std::vector<SimulationDelta>& deltas) const;
    bool equivalent(const SimulationRun& left, const SimulationRun& right) const;
};

struct SimulationValidation {
    bool valid{};
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

class SimulationValidator {
public:
    SimulationValidation validate(const SimulationRun& run, const SimulationParameters& parameters) const;
    std::string render(const SimulationValidation& validation) const;
};
