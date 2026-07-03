#include "internal.hpp"

namespace aegis::detail {

DiagnosticCatalog::DiagnosticCatalog() {
    rules_.push_back(DiagnosticRule{"frame.reject", 1, "warning", "frames were rejected"});
    rules_.push_back(DiagnosticRule{"quota.reject", 1, "warning", "quota denials observed"});
    rules_.push_back(DiagnosticRule{"recovery.apply", 1024, "warning", "large recovery batch"});
    rules_.push_back(DiagnosticRule{"topology.summary", 4096, "warning", "large topology summary"});
    rules_.push_back(DiagnosticRule{"audit.length", 8192, "warning", "large diagnostics payload"});
}

std::vector<Alert> DiagnosticCatalog::evaluate(std::string_view key, std::int64_t value) const {
    const auto normalized = canonical_text(key);
    std::vector<Alert> result;
    for (const auto& rule : rules_) {
        if (rule.key != normalized || value < rule.threshold) continue;
        result.push_back(Alert{0U, "diagnostics", rule.severity, rule.message + ":" + std::to_string(value), false});
    }
    return result;
}

std::vector<DiagnosticRule> DiagnosticCatalog::rules() const { return rules_; }

}  // namespace aegis::detail
