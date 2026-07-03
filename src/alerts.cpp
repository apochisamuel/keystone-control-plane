#include "internal.hpp"

namespace aegis::detail {

std::uint64_t AlertRouter::raise(std::string source, std::string severity, std::string message) {
    Alert alert;
    alert.id = next_id_++;
    alert.source = canonical_text(source.empty() ? "engine" : source);
    alert.severity = canonical_text(severity.empty() ? "warning" : severity);
    alert.message = std::move(message);
    alerts_.push_back(std::move(alert));
    return alerts_.back().id;
}

bool AlertRouter::acknowledge(std::uint64_t id) {
    for (auto& alert : alerts_) if (alert.id == id && !alert.acknowledged) { alert.acknowledged = true; return true; }
    return false;
}

std::vector<Alert> AlertRouter::outstanding() const {
    std::vector<Alert> result;
    for (const auto& alert : alerts_) if (!alert.acknowledged) result.push_back(alert);
    return result;
}

std::vector<Alert> AlertRouter::by_severity(std::string_view severity) const {
    const auto expected = canonical_text(severity);
    std::vector<Alert> result;
    for (const auto& alert : alerts_) if (alert.severity == expected) result.push_back(alert);
    return result;
}

std::string AlertRouter::report() const {
    std::unordered_map<std::string, std::size_t> counts;
    for (const auto& alert : outstanding()) counts[alert.severity]++;
    std::ostringstream output;
    output << "alerts.outstanding=" << outstanding().size() << ";";
    for (const auto& [severity, count] : counts) output << severity << "=" << count << ";";
    return output.str();
}

void AlertRouter::clear() { next_id_ = 1; alerts_.clear(); }

}  // namespace aegis::detail
