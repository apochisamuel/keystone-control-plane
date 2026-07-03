#include "internal.hpp"

namespace aegis::detail {

void PolicyConflictMatrix::note(const PolicyCandidate& candidate){candidates_[candidate.session_id]=candidate;}

std::vector<PolicyConflict> PolicyConflictMatrix::conflicts() const {std::vector<PolicyConflict> out;std::unordered_map<std::uint32_t,std::uint32_t> routes;for(const auto&[session,candidate]:candidates_){if(candidate.route_id==0U)out.push_back({session,"missing route"});if(const auto found=routes.find(candidate.route_id);found!=routes.end()&&found->second!=session)out.push_back({session,"route shared with "+std::to_string(found->second)});else routes[candidate.route_id]=session;}return out;}

void PolicyConflictMatrix::clear(){candidates_.clear();}

}  // namespace aegis::detail
