#include "internal.hpp"

namespace aegis::detail {

void FrameFingerprintIndex::add(std::uint32_t session_id,std::string fingerprint){if(fingerprint.empty())return;all_.insert(fingerprint);auto& values=by_session_[session_id];if(std::find(values.begin(),values.end(),fingerprint)==values.end())values.push_back(std::move(fingerprint));}

bool FrameFingerprintIndex::contains(std::string_view fingerprint) const{return all_.contains(std::string(fingerprint));}

std::vector<std::string> FrameFingerprintIndex::for_session(std::uint32_t session_id) const {const auto found=by_session_.find(session_id);return found==by_session_.end()?std::vector<std::string>{}:found->second;}

void FrameFingerprintIndex::clear(){by_session_.clear();all_.clear();}

}  // namespace aegis::detail
