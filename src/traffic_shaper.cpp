#include "internal.hpp"

namespace aegis::detail {

void TrafficShaper::configure(std::uint32_t session_id, std::uint64_t allowance, std::uint64_t refill) {
    auto& state = states_[session_id];
    state.allowance = allowance;
    state.max = allowance;
    state.refill = refill;
}

TrafficDecision TrafficShaper::admit(std::uint32_t session_id, std::uint64_t units, std::uint64_t tick) {
    TrafficDecision decision;
    auto& state = states_[session_id];
    if (state.max == 0U) { state.max = 1024U; state.allowance = 1024U; state.refill = 64U; }
    if (tick > state.tick) {
        const auto elapsed = tick - state.tick;
        const auto added = elapsed > 0U && state.refill > std::numeric_limits<std::uint64_t>::max() / elapsed
            ? std::numeric_limits<std::uint64_t>::max() : elapsed * state.refill;
        state.allowance = std::min(state.max, state.allowance + added);
        state.tick = tick;
    }
    if (units <= state.allowance) {
        state.allowance -= units;
        decision.allowed = true;
        decision.allowance = state.allowance;
        decision.reason = "admitted";
        return decision;
    }
    decision.allowed = false;
    decision.allowance = state.allowance;
    const auto missing = units - state.allowance;
    decision.delay = state.refill == 0U ? std::numeric_limits<std::uint64_t>::max() : (missing + state.refill - 1U) / state.refill;
    decision.reason = "shape-delay";
    return decision;
}

std::string TrafficShaper::report() const {
    std::ostringstream output;
    output << "shaper.sessions=" << states_.size() << ";";
    for (const auto& [id, state] : states_) output << id << ":allowance=" << state.allowance << ":max=" << state.max << ";";
    return output.str();
}

void TrafficShaper::clear() { states_.clear(); }

}  // namespace aegis::detail
