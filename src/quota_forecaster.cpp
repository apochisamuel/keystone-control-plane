#include "internal.hpp"

namespace aegis::detail {

void QuotaForecaster::observe(std::uint32_t session_id, std::uint64_t units) {
    auto& history = history_[session_id];
    history.push_back(units);
    if (history.size() > 32U) history.erase(history.begin());
}

QuotaProjection QuotaForecaster::project(std::uint32_t session_id, std::uint64_t ceiling) const {
    QuotaProjection projection;
    projection.session_id = session_id;
    projection.ceiling = ceiling;
    const auto iterator = history_.find(session_id);
    if (iterator == history_.end() || iterator->second.empty()) return projection;
    const auto& history = iterator->second;
    for (const auto sample : history) projection.observed += sample;
    const auto average = projection.observed / history.size();
    const auto last = history.back();
    const auto drift = last > average ? (last - average) / 2U : 0U;
    projection.forecast = average * 4U + drift;
    projection.exceeds = ceiling != 0U && projection.forecast > ceiling;
    return projection;
}

std::string QuotaForecaster::report() const {
    std::ostringstream output;
    output << "quota-forecast.sessions=" << history_.size() << ";";
    for (const auto& [id, samples] : history_) output << id << ":samples=" << samples.size() << ";";
    return output.str();
}

void QuotaForecaster::clear() { history_.clear(); }

}  // namespace aegis::detail
