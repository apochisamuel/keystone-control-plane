#include "internal.hpp"

namespace aegis::detail {

void KeyRotationLedger::rotate(std::string domain,std::string fingerprint,std::uint64_t epoch){if(!domain.empty()&&!fingerprint.empty())keys_[std::move(domain)]={std::move(fingerprint),epoch};}

std::optional<std::string> KeyRotationLedger::active(std::string_view domain) const {const auto found=keys_.find(std::string(domain));return found==keys_.end()?std::nullopt:std::optional<std::string>(found->second.fingerprint);}

std::vector<std::string> KeyRotationLedger::expiring(std::uint64_t before_epoch) const {std::vector<std::string> out;for(const auto&[domain,key]:keys_)if(key.epoch<before_epoch)out.push_back(domain);std::sort(out.begin(),out.end());return out;}

void KeyRotationLedger::clear(){keys_.clear();}

}  // namespace aegis::detail
