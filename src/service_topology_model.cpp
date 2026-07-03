#include "internal.hpp"

namespace aegis::detail {

void ServiceTopologyModel::upsert(std::string name, std::uint64_t capacity) {
    if (name.empty()) return;
    const auto stored_name = name;
    auto& node = nodes_[std::move(name)];
    node.name = stored_name;
    node.capacity = capacity;
    node.allocated = std::min(node.allocated, node.capacity);
}

void ServiceTopologyModel::connect(std::string left, std::string right) {
    if (left.empty() || right.empty() || left == right) return;
    auto& source = nodes_[left];
    source.name = left;
    source.neighbors.insert(right);
    auto& target = nodes_[right];
    target.name = right;
    target.neighbors.insert(left);
}

bool ServiceTopologyModel::allocate(std::string_view name, std::uint64_t amount) {
    const auto found = nodes_.find(std::string(name));
    if (found == nodes_.end() || amount > found->second.capacity - found->second.allocated) return false;
    found->second.allocated += amount;
    return true;
}

void ServiceTopologyModel::release(std::string_view name, std::uint64_t amount) {
    const auto found = nodes_.find(std::string(name));
    if (found == nodes_.end()) return;
    found->second.allocated = amount >= found->second.allocated ? 0U : found->second.allocated - amount;
}

std::vector<std::string> ServiceTopologyModel::path(std::string_view source, std::string_view destination) const {
    const auto source_name = std::string(source);
    const auto destination_name = std::string(destination);
    if (!nodes_.contains(source_name) || !nodes_.contains(destination_name)) return {};
    std::deque<std::string> queue{source_name};
    std::unordered_map<std::string, std::string> parent;
    parent[source_name] = {};
    while (!queue.empty()) {
        auto current = std::move(queue.front());
        queue.pop_front();
        if (current == destination_name) break;
        const auto found = nodes_.find(current);
        if (found == nodes_.end()) continue;
        for (const auto& neighbor : found->second.neighbors) {
            if (parent.contains(neighbor)) continue;
            parent[neighbor] = current;
            queue.push_back(neighbor);
        }
    }
    if (!parent.contains(destination_name)) return {};
    std::vector<std::string> result;
    for (auto current = destination_name; !current.empty(); current = parent[current]) result.push_back(current);
    std::reverse(result.begin(), result.end());
    return result;
}

std::string ServiceTopologyModel::report() const {
    std::vector<std::vector<std::string>> rows{{"name", "capacity", "allocated", "available", "neighbors"}};
    std::vector<std::string> names;
    for (const auto& [name, _] : nodes_) names.push_back(name);
    std::sort(names.begin(), names.end());
    for (const auto& name : names) {
        const auto& node = nodes_.at(name);
        const std::vector<std::string> neighbors(node.neighbors.begin(), node.neighbors.end());
        rows.push_back({name, std::to_string(node.capacity), std::to_string(node.allocated),
                        std::to_string(node.capacity - node.allocated), join_labels(neighbors, ",")});
    }
    return render_table(rows);
}

void ServiceTopologyModel::clear() { nodes_.clear(); }

}  // namespace aegis::detail
