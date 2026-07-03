#include "internal.hpp"

namespace aegis::detail {

void UsageAggregator::record(std::uint32_t session_id, std::uint64_t generation,
                             std::size_t payload_bytes, std::string route_key, bool accepted) {
    auto& bucket = buckets_[session_id];
    bucket.session_id = session_id;
    if (bucket.generation != generation) {
        bucket.generation = generation;
        bucket.messages = 0;
        bucket.payload_bytes = 0;
        bucket.decisions = 0;
        bucket.rejected = 0;
        bucket.route_keys.clear();
    }
    bucket.messages++;
    if (payload_bytes > std::numeric_limits<std::uint64_t>::max() - bucket.payload_bytes) {
        bucket.payload_bytes = std::numeric_limits<std::uint64_t>::max();
    } else {
        bucket.payload_bytes += payload_bytes;
    }
    if (accepted) bucket.decisions++;
    else bucket.rejected++;
    if (!route_key.empty()) bucket.route_keys.insert(canonical_text(route_key));
}

std::optional<UsageBucket> UsageAggregator::bucket(std::uint32_t session_id) const {
    const auto iterator = buckets_.find(session_id);
    return iterator == buckets_.end() ? std::nullopt : std::optional<UsageBucket>(iterator->second);
}

std::vector<UsageBucket> UsageAggregator::all() const {
    std::vector<UsageBucket> result;
    for (const auto& [id, bucket] : buckets_) { (void)id; result.push_back(bucket); }
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) { return left.session_id < right.session_id; });
    return result;
}

std::string UsageAggregator::report() const {
    std::ostringstream output;
    output << "usage.sessions=" << buckets_.size() << ";";
    for (const auto& bucket : all()) {
        output << bucket.session_id << ":m=" << bucket.messages << ":b=" << bucket.payload_bytes
               << ":ok=" << bucket.decisions << ":reject=" << bucket.rejected
               << ":routes=" << bucket.route_keys.size() << ";";
    }
    return output.str();
}

void UsageAggregator::clear() { buckets_.clear(); }

}  // namespace aegis::detail
