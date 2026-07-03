#include "internal.hpp"

namespace aegis::detail {

void RateLimiter::configure(std::string key, double capacity, double refill_per_tick) {
    key = canonical_text(key);
    if (key.empty()) return;
    capacity = std::max(0.0, capacity);
    refill_per_tick = std::max(0.0, refill_per_tick);
    auto& bucket = buckets_[key];
    bucket.capacity = capacity;
    bucket.tokens = capacity;
    bucket.refill_per_tick = refill_per_tick;
}

bool RateLimiter::consume(std::string_view key, double cost, std::uint64_t tick) {
    auto normalized = canonical_text(key);
    auto iterator = buckets_.find(normalized);
    if (iterator == buckets_.end()) {
        configure(normalized, 256.0, 4.0);
        iterator = buckets_.find(normalized);
    }
    auto& bucket = iterator->second;
    if (tick > bucket.last_tick) {
        const auto elapsed = static_cast<double>(tick - bucket.last_tick);
        bucket.tokens = std::min(bucket.capacity, bucket.tokens + elapsed * bucket.refill_per_tick);
        bucket.last_tick = tick;
    }
    if (cost < 0.0 || cost > bucket.tokens) return false;
    bucket.tokens -= cost;
    return true;
}

std::optional<RateBucket> RateLimiter::bucket(std::string_view key) const {
    const auto iterator = buckets_.find(canonical_text(key));
    return iterator == buckets_.end() ? std::nullopt : std::optional<RateBucket>(iterator->second);
}

std::string RateLimiter::report() const {
    std::vector<std::string> keys;
    for (const auto& [key, bucket] : buckets_) { (void)bucket; keys.push_back(key); }
    std::sort(keys.begin(), keys.end());
    std::ostringstream output;
    output << "rate.buckets=" << keys.size() << ";";
    for (const auto& key : keys) {
        const auto& bucket = buckets_.at(key);
        output << key << ":tokens=" << static_cast<std::uint64_t>(bucket.tokens) << ";";
    }
    return output.str();
}

void RateLimiter::clear() { buckets_.clear(); }

}  // namespace aegis::detail
