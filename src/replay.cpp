#include "internal.hpp"

namespace aegis::detail {

bool ReplayGuard::accept(std::uint32_t session_id, std::uint32_t sequence) {
    auto& window = windows_[session_id];
    if (!window.initialized) {
        window.initialized = true;
        window.high = sequence;
        window.bitmap = 1U;
        return true;
    }
    if (sequence > window.high) {
        const auto delta = sequence - window.high;
        if (delta >= 64U) window.bitmap = 1U;
        else window.bitmap = (window.bitmap << delta) | 1U;
        window.high = sequence;
        return true;
    }
    const auto delta = window.high - sequence;
    if (delta >= 64U) {
        ++rejected_;
        return false;
    }
    const auto mask = std::uint64_t{1} << delta;
    if ((window.bitmap & mask) != 0U) {
        ++rejected_;
        return false;
    }
    window.bitmap |= mask;
    return true;
}

void ReplayGuard::reset(std::uint32_t session_id) {
    windows_.erase(session_id);
}

std::size_t ReplayGuard::rejected() const {
    return rejected_;
}

std::vector<std::uint32_t> ReplayGuard::observed(std::uint32_t session_id) const {
    const auto iterator = windows_.find(session_id);
    if (iterator == windows_.end() || !iterator->second.initialized) return {};
    std::vector<std::uint32_t> result;
    for (std::uint32_t delta = 0; delta < 64U; ++delta) {
        if ((iterator->second.bitmap & (std::uint64_t{1} << delta)) != 0U && iterator->second.high >= delta) {
            result.push_back(iterator->second.high - delta);
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

}  // namespace aegis::detail
