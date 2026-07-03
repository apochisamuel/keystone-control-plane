#include "internal.hpp"

namespace aegis::detail {

void ScenarioControlService::observe(const Frame& frame, bool accepted) {
    if (!accepted) {
        Frame rejected = frame;
        rejected.flags = static_cast<Byte>(rejected.flags | 0x80U);
        frames_.push_back(std::move(rejected));
    } else {
        frames_.push_back(frame);
    }
    if (frames_.size() > 96U) frames_.erase(frames_.begin(), frames_.begin() + static_cast<std::ptrdiff_t>(frames_.size() - 96U));
}

std::string ScenarioControlService::report() const {
    const auto steps = builder_.build(frames_);
    const auto costs = costs_.estimate(steps);
    const auto findings = checker_.validate(steps);
    return renderer_.render_steps(steps) + "\n" + renderer_.render_costs(costs) + "\n" + renderer_.render_findings(findings);
}

std::vector<std::string> ScenarioControlService::findings() const { return checker_.validate(builder_.build(frames_)); }

void ScenarioControlService::clear() { frames_.clear(); }

}  // namespace aegis::detail
