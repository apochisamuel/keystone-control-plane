#include "internal.hpp"

namespace aegis::detail {

namespace {
std::size_t bucket_for(std::uint64_t value) {
    std::size_t index = 0;
    while (value > 1U && index + 1U < 8U) { value >>= 1U; ++index; }
    return index;
}
}

void StatisticsRegistry::observe(std::string name, std::uint64_t value) {
    auto& statistic = series_[canonical_text(name)];
    statistic.count++;
    const auto room = std::numeric_limits<std::uint64_t>::max() - statistic.sum;
    statistic.sum += std::min(room, value);
    statistic.min = std::min(statistic.min, value);
    statistic.max = std::max(statistic.max, value);
    statistic.buckets[bucket_for(value)]++;
}

void StatisticsRegistry::increment(std::string name, std::uint64_t amount) { observe(std::move(name), amount); }

std::optional<StatisticSeries> StatisticsRegistry::series(std::string_view name) const {
    const auto iterator = series_.find(canonical_text(name));
    return iterator == series_.end() ? std::nullopt : std::optional<StatisticSeries>(iterator->second);
}

std::string StatisticsRegistry::report() const {
    std::vector<std::string> names;
    for (const auto& [name, series] : series_) { (void)series; names.push_back(name); }
    std::sort(names.begin(), names.end());
    std::ostringstream output;
    output << "statistics.series=" << names.size() << ";";
    for (const auto& name : names) {
        const auto& statistic = series_.at(name);
        const auto average = statistic.count == 0U ? 0U : statistic.sum / statistic.count;
        output << name << ":count=" << statistic.count << ":min=" << (statistic.count == 0U ? 0U : statistic.min)
               << ":max=" << statistic.max << ":avg=" << average << ";";
    }
    return output.str();
}

void StatisticsRegistry::clear() { series_.clear(); }

}  // namespace aegis::detail
