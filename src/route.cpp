#include "internal.hpp"

namespace aegis::detail {

void RoutingTable::install(Route route) {
    route.next_hop = canonical_text(route.next_hop);
    for (auto& label : route.labels) label = canonical_text(label);
    std::sort(route.labels.begin(), route.labels.end());
    route.labels.erase(std::unique(route.labels.begin(), route.labels.end()), route.labels.end());
    routes_[route.id] = std::move(route);
}

std::optional<Route> RoutingTable::find(std::uint32_t id) const {
    const auto iterator = routes_.find(id);
    if (iterator == routes_.end()) return std::nullopt;
    return iterator->second;
}

std::vector<Route> RoutingTable::select(std::string_view label) const {
    const auto key = canonical_text(label);
    std::vector<Route> result;
    for (const auto& [id, route] : routes_) {
        (void)id;
        if (!route.enabled) continue;
        if (key.empty() || std::binary_search(route.labels.begin(), route.labels.end(), key)) result.push_back(route);
    }
    std::sort(result.begin(), result.end(), [](const Route& left, const Route& right) {
        if (left.weight != right.weight) return left.weight > right.weight;
        return left.id < right.id;
    });
    return result;
}

void RoutingTable::erase(std::uint32_t id) {
    routes_.erase(id);
}

std::size_t RoutingTable::size() const {
    return routes_.size();
}

std::vector<Route> RoutingTable::all() const {
    std::vector<Route> result;
    result.reserve(routes_.size());
    for (const auto& [id, route] : routes_) {
        (void)id;
        result.push_back(route);
    }
    std::sort(result.begin(), result.end(), [](const Route& left, const Route& right) {
        return left.id < right.id;
    });
    return result;
}

void RouteIndex::rebuild(const std::vector<Route>& routes) {
    index_.clear();
    for (const auto& route : routes) {
        if (!route.enabled) continue;
        for (const auto& label : route.labels) index_[label].push_back(route.id);
    }
    for (auto& [label, ids] : index_) {
        (void)label;
        std::sort(ids.begin(), ids.end());
        ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    }
}

std::vector<std::uint32_t> RouteIndex::lookup(std::string_view label) const {
    const auto iterator = index_.find(canonical_text(label));
    return iterator == index_.end() ? std::vector<std::uint32_t>{} : iterator->second;
}

void RouteIndex::clear() {
    index_.clear();
}

}  // namespace aegis::detail
