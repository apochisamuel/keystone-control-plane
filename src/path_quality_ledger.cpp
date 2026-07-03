#include "internal.hpp"

namespace aegis::detail {

void PathQualityLedger::observe(std::string path,std::uint64_t score){if(!path.empty())scores_[std::move(path)]=score;}

std::optional<std::string> PathQualityLedger::best() const {if(scores_.empty())return std::nullopt;const auto found=std::min_element(scores_.begin(),scores_.end(),[](const auto&a,const auto&b){return a.second==b.second?a.first<b.first:a.second<b.second;});return found->first;}

std::vector<std::pair<std::string,std::uint64_t>> PathQualityLedger::history() const {return {scores_.begin(),scores_.end()};}

void PathQualityLedger::clear(){scores_.clear();}

}  // namespace aegis::detail
