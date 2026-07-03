#include "internal.hpp"

namespace aegis::detail {

ErrorCatalog::ErrorCatalog() {
    entries_.emplace("frame-rejected", ErrorDescriptor{"frame-rejected", "ingress", "warning", "inspect frame encoding"});
    entries_.emplace("quota-denied", ErrorDescriptor{"quota-denied", "quota", "warning", "increase allowance or retry later"});
    entries_.emplace("policy-denied", ErrorDescriptor{"policy-denied", "policy", "warning", "review policy facts"});
    entries_.emplace("recovery-failed", ErrorDescriptor{"recovery-failed", "recovery", "critical", "restore a verified checkpoint"});
    entries_.emplace("handover-failed", ErrorDescriptor{"handover-failed", "mobility", "warning", "abort and retain origin route"});
}

std::optional<ErrorDescriptor> ErrorCatalog::find(std::string_view code) const {
    const auto iterator = entries_.find(canonical_text(code));
    return iterator == entries_.end() ? std::nullopt : std::optional<ErrorDescriptor>(iterator->second);
}

std::vector<ErrorDescriptor> ErrorCatalog::by_category(std::string_view category) const {
    const auto expected = canonical_text(category);
    std::vector<ErrorDescriptor> result;
    for (const auto& [code, descriptor] : entries_) { (void)code; if (descriptor.category == expected) result.push_back(descriptor); }
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) { return left.code < right.code; });
    return result;
}

std::string ErrorCatalog::describe(std::string_view code) const {
    const auto descriptor = find(code);
    if (!descriptor.has_value()) return "unknown-error";
    return descriptor->severity + ":" + descriptor->category + ":" + descriptor->remediation;
}

}  // namespace aegis::detail
