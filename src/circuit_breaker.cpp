#include "internal.hpp"

namespace aegis::detail {

bool CircuitBreaker::allow(std::string_view key, std::uint64_t tick) const {
    const auto iterator = states_.find(canonical_text(key));
    if (iterator == states_.end()) return true;
    return !iterator->second.open || tick >= iterator->second.opened_until;
}

void CircuitBreaker::record_success(std::string key) {
    auto& state = states_[canonical_text(key)];
    state.successes++;
    if (state.successes >= 2U) {
        state.failures = 0;
        state.open = false;
        state.opened_until = 0;
    }
}

void CircuitBreaker::record_failure(std::string key, std::uint64_t tick, std::uint32_t threshold,
                                    std::uint64_t cooldown) {
    auto& state = states_[canonical_text(key)];
    state.failures++;
    state.successes = 0;
    if (state.failures >= std::max(1U, threshold)) {
        state.open = true;
        state.opened_until = tick + cooldown;
    }
}

std::optional<BreakerState> CircuitBreaker::state(std::string_view key) const {
    const auto iterator = states_.find(canonical_text(key));
    return iterator == states_.end() ? std::nullopt : std::optional<BreakerState>(iterator->second);
}

std::string CircuitBreaker::report() const {
    std::ostringstream output;
    output << "breaker.keys=" << states_.size() << ";";
    for (const auto& [key, state] : states_) output << key << ":open=" << state.open << ":failures=" << state.failures << ";";
    return output.str();
}

void CircuitBreaker::clear() { states_.clear(); }

}  // namespace aegis::detail
