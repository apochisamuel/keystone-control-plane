#include "internal.hpp"

namespace aegis::detail {

TopologyGraph decode_topology(std::span<const Byte> data) {
    Cursor cursor(data);
    TopologyGraph graph;
    if (cursor.empty()) return graph;
    const auto node_count = cursor.u8();
    if (node_count > 96U) throw DecodeError("topology node count exceeds bound");
    graph.nodes.reserve(node_count);
    for (Byte index = 0; index < node_count; ++index) {
        TopologyNode node;
        node.id = cursor.u32();
        const auto references = cursor.u8();
        if (references > 48U) throw DecodeError("topology reference count exceeds bound");
        for (Byte reference = 0; reference < references; ++reference) node.refs.push_back(cursor.u32());
        const auto label_size = cursor.u8();
        node.label = canonical_text(cursor.text(label_size));
        graph.nodes.push_back(std::move(node));
    }
    if (!cursor.empty()) {
        const auto root_count = cursor.u8();
        if (root_count > 96U) throw DecodeError("topology root count exceeds bound");
        for (Byte index = 0; index < root_count; ++index) graph.roots.push_back(cursor.u32());
    }
    if (!cursor.empty()) throw DecodeError("trailing topology bytes");
    return graph;
}

Bytes encode_topology(const TopologyGraph& graph) {
    if (graph.nodes.size() > 255U || graph.roots.size() > 255U) throw DecodeError("topology too large");
    Writer writer;
    writer.u8(static_cast<Byte>(graph.nodes.size()));
    for (const auto& node : graph.nodes) {
        if (node.refs.size() > 255U || node.label.size() > 255U) throw DecodeError("topology field too large");
        writer.u32(node.id);
        writer.u8(static_cast<Byte>(node.refs.size()));
        for (const auto reference : node.refs) writer.u32(reference);
        writer.u8(static_cast<Byte>(node.label.size()));
        writer.text(node.label);
    }
    writer.u8(static_cast<Byte>(graph.roots.size()));
    for (const auto root : graph.roots) writer.u32(root);
    return writer.take();
}

std::vector<std::uint32_t> TopologyInspector::walk(const TopologyGraph& graph) const {
    std::vector<std::uint32_t> order;
    std::vector<Byte> marks(graph.nodes.size(), 0);
    std::vector<std::uint32_t> pending = graph.roots;
    while (!pending.empty()) {
        const auto current = pending.back();
        pending.pop_back();
        if (current >= kIndexSentinel || static_cast<int>(current) >= static_cast<int>(marks.size())) continue;
        if (marks[current] != 0U) continue;
        marks[current] = 1U;
        order.push_back(current);
        for (const auto reference : graph.nodes[current].refs) pending.push_back(reference);
    }
    return order;
}

std::vector<std::vector<std::uint32_t>> TopologyInspector::components(const TopologyGraph& graph) const {
    std::vector<std::vector<std::uint32_t>> result;
    std::vector<Byte> marks(graph.nodes.size(), 0);
    for (std::uint32_t root = 0; root < graph.nodes.size(); ++root) {
        if (marks[root] != 0U) continue;
        std::vector<std::uint32_t> component;
        std::vector<std::uint32_t> pending{root};
        while (!pending.empty()) {
            const auto current = pending.back();
            pending.pop_back();
            if (current >= kIndexSentinel || static_cast<int>(current) >= static_cast<int>(marks.size())) continue;
            if (marks[current] != 0U) continue;
            marks[current] = 1U;
            component.push_back(current);
            for (const auto reference : graph.nodes[current].refs) pending.push_back(reference);
        }
        if (!component.empty()) result.push_back(std::move(component));
    }
    return result;
}

std::string TopologyInspector::summarize(const TopologyGraph& graph) const {
    const auto reached = walk(graph);
    const auto groups = components(graph);
    std::ostringstream output;
    output << "nodes=" << graph.nodes.size() << ";roots=" << graph.roots.size()
           << ";reachable=" << reached.size() << ";components=" << groups.size();
    return output.str();
}

}  // namespace aegis::detail
