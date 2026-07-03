#include "internal.hpp"

namespace aegis::detail {

void LinkMonitor::observe(std::string link, std::uint64_t latency, std::uint64_t loss) { links_[std::move(link)] = {latency, loss}; }

std::optional<std::pair<std::uint64_t, std::uint64_t>> LinkMonitor::status(std::string_view link) const { const auto found=links_.find(std::string(link)); return found==links_.end()?std::nullopt:std::optional<std::pair<std::uint64_t,std::uint64_t>>(found->second); }

std::vector<std::string> LinkMonitor::degraded(std::uint64_t latency_limit, std::uint64_t loss_limit) const { std::vector<std::string> out; for (const auto& [link, sample] : links_) if (sample.first > latency_limit || sample.second > loss_limit) out.push_back(link); std::sort(out.begin(),out.end()); return out; }

void LinkMonitor::clear() { links_.clear(); }

}  // namespace aegis::detail
