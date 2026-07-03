#include "internal.hpp"

namespace aegis::detail {

void DependencyGraph::add(std::string component, std::string dependency) { if (!component.empty() && !dependency.empty()) edges_[std::move(component)].insert(std::move(dependency)); }

bool DependencyGraph::remove(std::string_view component, std::string_view dependency) {
    const auto found = edges_.find(std::string(component));
    if (found == edges_.end()) return false;
    const bool removed = found->second.erase(std::string(dependency)) != 0U;
    if (found->second.empty()) edges_.erase(found);
    return removed;
}

std::vector<std::string> DependencyGraph::closure(std::string_view component) const {
    std::vector<std::string> out; std::set<std::string> seen; std::vector<std::string> pending{std::string(component)};
    while (!pending.empty()) { auto current = std::move(pending.back()); pending.pop_back(); if (!seen.insert(current).second) continue; if (current != component) out.push_back(current); if (const auto found = edges_.find(current); found != edges_.end()) for (const auto& child : found->second) pending.push_back(child); }
    std::sort(out.begin(), out.end()); return out;
}

bool DependencyGraph::cyclic(std::string_view component) const {
    std::set<std::string> visiting, complete;
    std::function<bool(const std::string&)> visit = [&](const std::string& node) { if (visiting.contains(node)) return true; if (complete.contains(node)) return false; visiting.insert(node); if (const auto found = edges_.find(node); found != edges_.end()) for (const auto& child : found->second) if (visit(child)) return true; visiting.erase(node); complete.insert(node); return false; };
    return visit(std::string(component));
}

void DependencyGraph::clear() { edges_.clear(); }

}  // namespace aegis::detail
