#include "internal.hpp"

namespace aegis::detail {

void TunnelPathResolver::bind(std::uint32_t tunnel_id,std::vector<std::uint32_t> path){if(tunnel_id!=0U&&!path.empty())paths_[tunnel_id]=std::move(path);}

std::optional<std::vector<std::uint32_t>> TunnelPathResolver::resolve(std::uint32_t tunnel_id) const {const auto found=paths_.find(tunnel_id);return found==paths_.end()?std::nullopt:std::optional<std::vector<std::uint32_t>>(found->second);}

bool TunnelPathResolver::remove(std::uint32_t tunnel_id){return paths_.erase(tunnel_id)!=0U;}

void TunnelPathResolver::clear(){paths_.clear();}

}  // namespace aegis::detail
