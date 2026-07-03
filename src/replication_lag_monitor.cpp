#include "internal.hpp"

namespace aegis::detail {

void ReplicationLagMonitor::observe(std::string replica,std::uint64_t produced,std::uint64_t applied){if(!replica.empty())lags_[std::move(replica)]=produced>applied?produced-applied:0U;}

std::optional<std::uint64_t> ReplicationLagMonitor::lag(std::string_view replica) const {const auto found=lags_.find(std::string(replica));return found==lags_.end()?std::nullopt:std::optional<std::uint64_t>(found->second);}

std::vector<std::string> ReplicationLagMonitor::behind(std::uint64_t limit) const {std::vector<std::string> out;for(const auto&[replica,lag_value]:lags_)if(lag_value>limit)out.push_back(replica);std::sort(out.begin(),out.end());return out;}

void ReplicationLagMonitor::clear(){lags_.clear();}

}  // namespace aegis::detail
