#include "internal.hpp"

namespace aegis::detail {

void ProtocolVersionRegistry::register_version(std::string name,std::uint32_t min,std::uint32_t max){if(!name.empty()&&min<=max)ranges_[std::move(name)]={min,max};}

bool ProtocolVersionRegistry::supports(std::string_view name,std::uint32_t version) const {const auto found=ranges_.find(std::string(name));return found!=ranges_.end()&&version>=found->second.first&&version<=found->second.second;}

std::vector<std::string> ProtocolVersionRegistry::versions() const {std::vector<std::string> out;for(const auto&[name,range]:ranges_)out.push_back(name+":"+std::to_string(range.first)+"-"+std::to_string(range.second));std::sort(out.begin(),out.end());return out;}

void ProtocolVersionRegistry::clear(){ranges_.clear();}

}  // namespace aegis::detail
