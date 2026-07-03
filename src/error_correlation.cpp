#include "internal.hpp"

namespace aegis::detail {

void ErrorCorrelation::record(std::string code,std::uint32_t session_id){if(!code.empty())errors_[std::move(code)].push_back(session_id);}

std::vector<std::pair<std::string,std::uint64_t>> ErrorCorrelation::top(std::size_t limit) const {std::vector<std::pair<std::string,std::uint64_t>> out;for(const auto&[code,sessions]:errors_)out.push_back({code,sessions.size()});std::sort(out.begin(),out.end(),[](const auto&a,const auto&b){return a.second==b.second?a.first<b.first:a.second>b.second;});if(out.size()>limit)out.resize(limit);return out;}

std::vector<std::uint32_t> ErrorCorrelation::sessions(std::string_view code) const {const auto found=errors_.find(std::string(code));if(found==errors_.end())return {};auto out=found->second;std::sort(out.begin(),out.end());out.erase(std::unique(out.begin(),out.end()),out.end());return out;}

void ErrorCorrelation::clear(){errors_.clear();}

}  // namespace aegis::detail
