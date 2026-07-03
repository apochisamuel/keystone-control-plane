#include "internal.hpp"

namespace aegis::detail {

void Metrics::count(std::string key, std::uint64_t amount) {
    counters_[canonical_text(key)] += amount;
}

void Metrics::observe(std::string key, std::uint64_t value) {
    auto& sample = samples_[canonical_text(key)];
    if (sample.size() >= 128U) sample.erase(sample.begin());
    sample.push_back(value);
}

std::uint64_t Metrics::value(std::string_view key) const {
    const auto iterator = counters_.find(canonical_text(key));
    return iterator == counters_.end() ? 0U : iterator->second;
}

std::string Metrics::report() const {
    std::vector<std::pair<std::string, std::uint64_t>> counters(counters_.begin(), counters_.end());
    std::sort(counters.begin(), counters.end());
    std::ostringstream output;
    for (const auto& [key, value] : counters) output << key << '=' << value << '\n';
    std::vector<std::string> keys;
    for (const auto& [key, sample] : samples_) { (void)sample; keys.push_back(key); }
    std::sort(keys.begin(), keys.end());
    for (const auto& key : keys) {
        const auto& sample = samples_.at(key);
        std::uint64_t sum = 0;
        for (const auto value : sample) sum += value;
        output << key << ".count=" << sample.size() << ";avg=" << (sample.empty() ? 0U : sum / sample.size()) << '\n';
    }
    return output.str();
}

void Metrics::clear() {
    counters_.clear();
    samples_.clear();
}

}  // namespace aegis::detail
