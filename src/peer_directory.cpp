#include "internal.hpp"

namespace aegis::detail {

void PeerDirectory::register_peer(std::uint32_t id,std::string endpoint,std::string zone){if(id!=0U&&!endpoint.empty())peers_[id]={std::move(endpoint),std::move(zone)};}

std::optional<std::string> PeerDirectory::resolve(std::uint32_t id) const {const auto found=peers_.find(id);return found==peers_.end()?std::nullopt:std::optional<std::string>(found->second.endpoint);}

bool PeerDirectory::remove(std::uint32_t id){return peers_.erase(id)!=0U;}

std::vector<std::uint32_t> PeerDirectory::in_zone(std::string_view zone) const {std::vector<std::uint32_t> out;for(const auto&[id,peer]:peers_)if(peer.zone==zone)out.push_back(id);std::sort(out.begin(),out.end());return out;}

void PeerDirectory::clear(){peers_.clear();}

}  // namespace aegis::detail
