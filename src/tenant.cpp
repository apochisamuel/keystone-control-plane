#include "internal.hpp"

namespace aegis::detail {

bool TenantRegistry::register_tenant(TenantProfile profile) {
    if (profile.id == 0U || canonical_text(profile.name).empty() || profile.quota == 0U) return false;
    profile.name = canonical_text(profile.name);
    std::set<std::string> normalized;
    for (const auto& capability : profile.capabilities) {
        const auto item = canonical_text(capability);
        if (!item.empty()) normalized.insert(item);
    }
    profile.capabilities = std::move(normalized);
    return tenants_.emplace(profile.id, std::move(profile)).second;
}

std::optional<TenantProfile> TenantRegistry::find(std::uint32_t id) const {
    const auto iterator = tenants_.find(id);
    return iterator == tenants_.end() ? std::nullopt : std::optional<TenantProfile>(iterator->second);
}

bool TenantRegistry::enable(std::uint32_t id, bool enabled) {
    auto iterator = tenants_.find(id);
    if (iterator == tenants_.end()) return false;
    iterator->second.enabled = enabled;
    return true;
}

std::vector<TenantProfile> TenantRegistry::list() const {
    std::vector<TenantProfile> result;
    for (const auto& [id, profile] : tenants_) { (void)id; result.push_back(profile); }
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) { return left.id < right.id; });
    return result;
}

std::vector<std::string> TenantRegistry::validate(std::uint32_t id, std::string_view capability) const {
    std::vector<std::string> errors;
    const auto tenant = find(id);
    if (!tenant.has_value()) { errors.push_back("tenant-not-found"); return errors; }
    if (!tenant->enabled) errors.push_back("tenant-disabled");
    const auto requested = canonical_text(capability);
    if (!requested.empty() && !tenant->capabilities.contains(requested)) errors.push_back("capability-not-granted");
    return errors;
}

std::string TenantRegistry::report() const {
    std::ostringstream output;
    output << "tenants=" << tenants_.size() << ";";
    for (const auto& profile : list()) output << profile.id << ":" << profile.name << ":enabled=" << profile.enabled << ";";
    return output.str();
}

void TenantRegistry::clear() { tenants_.clear(); }

}  // namespace aegis::detail
