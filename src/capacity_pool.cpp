#include "internal.hpp"

namespace aegis::detail {

void CapacityPool::set_capacity(std::string resource, std::uint64_t capacity) {
    auto& slot = slots_[std::move(resource)];
    slot.capacity = capacity;
    slot.used = std::min(slot.used, slot.capacity);
}

bool CapacityPool::reserve(std::string_view resource, std::uint64_t amount) {
    const auto found = slots_.find(std::string(resource));
    if (found == slots_.end() || amount > found->second.capacity - found->second.used) return false;
    found->second.used += amount;
    return true;
}

void CapacityPool::release(std::string_view resource, std::uint64_t amount) {
    const auto found = slots_.find(std::string(resource));
    if (found != slots_.end()) found->second.used = amount >= found->second.used ? 0U : found->second.used - amount;
}

std::uint64_t CapacityPool::available(std::string_view resource) const {
    const auto found = slots_.find(std::string(resource));
    return found == slots_.end() ? 0U : found->second.capacity - found->second.used;
}

std::string CapacityPool::report() const {
    std::vector<std::vector<std::string>> rows{{"resource", "capacity", "used", "available"}};
    for (const auto& [name, slot] : slots_) rows.push_back({name, std::to_string(slot.capacity), std::to_string(slot.used), std::to_string(slot.capacity-slot.used)});
    return render_table(rows);
}

void CapacityPool::clear() { slots_.clear(); }

}  // namespace aegis::detail
