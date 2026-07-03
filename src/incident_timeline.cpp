#include "internal.hpp"

namespace aegis::detail {

void IncidentTimeline::append(std::uint64_t tick,std::string severity,std::string message){if(!message.empty())items_.push_back({tick,std::move(severity),std::move(message)});}

std::vector<std::string> IncidentTimeline::since(std::uint64_t tick) const {std::vector<std::string> out;for(const auto&item:items_)if(item.tick>=tick)out.push_back(std::to_string(item.tick)+":"+item.severity+":"+item.message);return out;}

void IncidentTimeline::clear(){items_.clear();}

}  // namespace aegis::detail
