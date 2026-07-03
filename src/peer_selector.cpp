#include "internal.hpp"

namespace aegis::detail {

void PeerSelector::upsert(PeerCandidate candidate){if(candidate.id!=0U)peers_[candidate.id]=std::move(candidate);}

std::optional<PeerCandidate> PeerSelector::select(std::string_view zone) const {std::optional<PeerCandidate> best;for(const auto&[_,peer]:peers_){if(!peer.healthy||peer.zone!=zone||peer.active>=peer.capacity)continue;if(!best||peer.active<best->active||(peer.active==best->active&&peer.id<best->id))best=peer;}return best;}

bool PeerSelector::release(std::uint32_t id,std::uint64_t units){auto found=peers_.find(id);if(found==peers_.end())return false;found->second.active=units>=found->second.active?0U:found->second.active-units;return true;}

void PeerSelector::clear(){peers_.clear();}

}  // namespace aegis::detail
