#include "internal.hpp"

namespace aegis::detail {

std::uint64_t HandoverPlanner::propose(std::uint32_t session_id, std::uint32_t source, std::uint32_t destination) {
    const auto id = next_id_++; plans_[id] = {id, session_id, source, destination, source == destination ? "rejected" : "proposed"}; return id;
}

bool HandoverPlanner::approve(std::uint64_t id) { auto found = plans_.find(id); if (found == plans_.end() || found->second.state != "proposed") return false; found->second.state = "approved"; return true; }

bool HandoverPlanner::complete(std::uint64_t id) { auto found = plans_.find(id); if (found == plans_.end() || found->second.state != "approved") return false; found->second.state = "complete"; return true; }

std::vector<HandoverPlan> HandoverPlanner::active(std::uint32_t session_id) const { std::vector<HandoverPlan> out; for (const auto& [_, plan] : plans_) if (plan.session_id == session_id && plan.state != "complete" && plan.state != "rejected") out.push_back(plan); std::sort(out.begin(), out.end(), [](const auto& a,const auto& b){ return a.id < b.id;}); return out; }

void HandoverPlanner::clear() { plans_.clear(); next_id_ = 1U; }

}  // namespace aegis::detail
