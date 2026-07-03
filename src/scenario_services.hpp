#pragma once

struct ScenarioStep {
    std::size_t index{};
    std::uint32_t session_id{};
    Operation operation{Operation::Heartbeat};
    std::string phase_before;
    std::string phase_after;
    std::uint64_t estimated_units{};
    bool accepted{};
    std::vector<std::string> notes;
};

class ScenarioTimelineBuilder {
public:
    std::vector<ScenarioStep> build(const std::vector<Frame>& frames) const;
    std::string phase_name(Operation operation) const;
};

class ScenarioInvariantChecker {
public:
    std::vector<std::string> validate(const std::vector<ScenarioStep>& steps) const;
    std::map<std::uint32_t, std::string> final_phases(const std::vector<ScenarioStep>& steps) const;
};

struct ScenarioCost { std::uint32_t session_id{}; std::uint64_t control_units{}; std::uint64_t data_units{}; std::uint64_t total{}; };
class ScenarioCostModel {
public:
    std::vector<ScenarioCost> estimate(const std::vector<ScenarioStep>& steps) const;
    std::uint64_t aggregate(const std::vector<ScenarioCost>& costs) const;
};

class ScenarioReportRenderer {
public:
    std::string render_steps(const std::vector<ScenarioStep>& steps) const;
    std::string render_costs(const std::vector<ScenarioCost>& costs) const;
    std::string render_findings(const std::vector<std::string>& findings) const;
};

class ScenarioControlService {
public:
    void observe(const Frame& frame, bool accepted);
    std::string report() const;
    std::vector<std::string> findings() const;
    void clear();
private:
    std::vector<Frame> frames_;
    ScenarioTimelineBuilder builder_;
    ScenarioInvariantChecker checker_;
    ScenarioCostModel costs_;
    ScenarioReportRenderer renderer_;
};
