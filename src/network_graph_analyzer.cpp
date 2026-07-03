#include "internal.hpp"

namespace aegis::detail {

std::vector<std::uint32_t> NetworkGraphAnalyzer::hubs(const TopologyGraph& graph,std::size_t minimum_degree) const {std::vector<std::uint32_t> out;for(const auto& node:graph.nodes)if(node.refs.size()>=minimum_degree)out.push_back(node.id);std::sort(out.begin(),out.end());return out;}

std::vector<std::uint32_t> NetworkGraphAnalyzer::leaves(const TopologyGraph& graph) const {std::vector<std::uint32_t> out;for(const auto& node:graph.nodes)if(node.refs.empty())out.push_back(node.id);std::sort(out.begin(),out.end());return out;}

std::string NetworkGraphAnalyzer::render(const TopologyGraph& graph) const {std::vector<std::vector<std::string>> rows{{"node","degree","label"}};for(const auto& node:graph.nodes)rows.push_back({std::to_string(node.id),std::to_string(node.refs.size()),node.label});return render_table(rows);}

}  // namespace aegis::detail
