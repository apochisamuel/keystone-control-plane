#include "internal.hpp"

namespace aegis::detail {

std::vector<std::string> TopologyValidator::validate(const TopologyGraph& graph) const {
    std::vector<std::string> errors;
    std::unordered_set<std::uint32_t> ids;
    for (std::size_t index = 0; index < graph.nodes.size(); ++index) {
        const auto& node = graph.nodes[index];
        if (!ids.insert(node.id).second) errors.push_back("duplicate node id:" + std::to_string(node.id));
        if (node.label.empty()) errors.push_back("empty node label:" + std::to_string(index));
        for (const auto reference : node.refs) {
            if (reference >= graph.nodes.size() && reference < kIndexSentinel) errors.push_back("reference out of range:" + std::to_string(reference));
        }
    }
    for (const auto root : graph.roots) {
        if (root >= graph.nodes.size() && root < kIndexSentinel) errors.push_back("root out of range:" + std::to_string(root));
    }
    return errors;
}

std::vector<std::uint32_t> TopologyValidator::roots(const TopologyGraph& graph) const {
    std::vector<std::uint32_t> result;
    for (const auto root : graph.roots) if (root < graph.nodes.size()) result.push_back(root);
    if (result.empty() && !graph.nodes.empty()) result.push_back(0U);
    return result;
}

std::string TopologyValidator::render(const TopologyGraph& graph) const {
    std::ostringstream output;
    output << "nodes=" << graph.nodes.size() << ";roots=" << join_labels([&] {
        std::vector<std::string> labels;
        for (const auto root : roots(graph)) labels.push_back(std::to_string(root));
        return labels;
    }(), ",") << ";";
    for (const auto& error : validate(graph)) output << "error=" << error << ";";
    return output.str();
}

}  // namespace aegis::detail
