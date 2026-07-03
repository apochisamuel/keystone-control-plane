#include "internal.hpp"

namespace aegis::detail {

void HealthMonitor::observe(std::string source, std::string name, std::int64_t value, std::string detail) {
    HealthSignal signal;
    signal.tick = ++tick_;
    signal.source = canonical_text(source.empty() ? "engine" : source);
    signal.name = canonical_text(name.empty() ? "signal" : name);
    signal.value = value;
    signal.detail = std::move(detail);
    signals_.push_back(std::move(signal));
    constexpr std::size_t kRetain = 512U;
    if (signals_.size() > kRetain) signals_.erase(signals_.begin(), signals_.begin() + 64);
}

void HealthMonitor::mark(std::string component, bool healthy, std::string reason) {
    auto key = canonical_text(component.empty() ? "engine" : component);
    marks_[key].emplace_back(healthy, canonical_text(reason.empty() ? (healthy ? "ok" : "degraded") : reason));
    if (marks_[key].size() > 32U) marks_[key].erase(marks_[key].begin());
}

HealthStatus HealthMonitor::evaluate(std::string_view component) const {
    HealthStatus status;
    status.component = canonical_text(component.empty() ? "engine" : component);
    status.state = "healthy";
    status.score = 100;
    const auto iterator = marks_.find(status.component);
    if (iterator != marks_.end()) {
        for (const auto& [healthy, reason] : iterator->second) {
            if (healthy) {
                status.score = std::min<std::int64_t>(100, status.score + 1);
            } else {
                status.score -= 25;
                status.reasons.push_back(reason);
            }
        }
    }
    for (const auto& signal : signals_) {
        if (signal.source != status.component) continue;
        if (signal.name.find("reject") != std::string::npos && signal.value > 0) {
            status.score -= std::min<std::int64_t>(20, signal.value);
            status.reasons.push_back(signal.name + "=" + std::to_string(signal.value));
        }
        if (signal.name.find("latency") != std::string::npos && signal.value > 1000) {
            status.score -= 10;
            status.reasons.push_back("latency-threshold");
        }
    }
    if (status.score < 0) status.score = 0;
    if (status.score < 40) status.state = "critical";
    else if (status.score < 75) status.state = "degraded";
    if (status.reasons.empty()) status.reasons.push_back("nominal");
    return status;
}

std::vector<HealthStatus> HealthMonitor::all() const {
    std::unordered_set<std::string> names;
    for (const auto& [name, marks] : marks_) { (void)marks; names.insert(name); }
    for (const auto& signal : signals_) names.insert(signal.source);
    if (names.empty()) names.insert("engine");
    std::vector<std::string> ordered(names.begin(), names.end());
    std::sort(ordered.begin(), ordered.end());
    std::vector<HealthStatus> result;
    for (const auto& name : ordered) result.push_back(evaluate(name));
    return result;
}

std::vector<HealthSignal> HealthMonitor::recent(std::size_t limit) const {
    if (limit == 0U) return {};
    const auto first = signals_.size() > limit ? signals_.size() - limit : 0U;
    return {signals_.begin() + static_cast<std::ptrdiff_t>(first), signals_.end()};
}

std::string HealthMonitor::report() const {
    std::ostringstream output;
    for (const auto& status : all()) {
        output << "health." << status.component << "=" << status.state << ":" << status.score << ";";
    }
    return output.str();
}

void HealthMonitor::clear() {
    tick_ = 0;
    signals_.clear();
    marks_.clear();
}

}  // namespace aegis::detail
