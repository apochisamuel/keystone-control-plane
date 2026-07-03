#include "internal.hpp"

namespace aegis::detail {

void CongestionEstimator::observe(std::string path, std::uint64_t offered, std::uint64_t delivered) {
    auto& sample = samples_[std::move(path)];
    sample.offered += offered;
    sample.delivered += std::min(offered, delivered);
    ++sample.samples;
}

std::uint64_t CongestionEstimator::score(std::string_view path) const {
    const auto found = samples_.find(std::string(path));
    if (found == samples_.end() || found->second.offered == 0U) return 0U;
    return ((found->second.offered - found->second.delivered) * 100U) / found->second.offered;
}

std::string CongestionEstimator::advice(std::string_view path) const {
    const auto value = score(path);
    if (value >= 50U) return "drain";
    if (value >= 20U) return "slow";
    return "steady";
}

void CongestionEstimator::clear() { samples_.clear(); }

}  // namespace aegis::detail
