#include "internal.hpp"

namespace aegis::detail {

std::uint64_t TraceBuffer::begin(std::string name, std::uint64_t parent) {
    TraceSpan span;
    span.id = next_id_++;
    span.parent = parent;
    span.name = canonical_text(name.empty() ? "operation" : name);
    span.start = ++tick_;
    spans_.emplace(span.id, std::move(span));
    return next_id_ - 1U;
}

bool TraceBuffer::annotate(std::uint64_t id, std::string key, std::string value) {
    const auto iterator = spans_.find(id);
    if (iterator == spans_.end() || iterator->second.end != 0U) return false;
    key = canonical_text(key);
    if (key.empty()) return false;
    iterator->second.attributes[std::move(key)] = std::move(value);
    return true;
}

bool TraceBuffer::end(std::uint64_t id) {
    const auto iterator = spans_.find(id);
    if (iterator == spans_.end() || iterator->second.end != 0U) return false;
    iterator->second.end = ++tick_;
    return true;
}

std::vector<TraceSpan> TraceBuffer::complete() const {
    std::vector<TraceSpan> result;
    for (const auto& [id, span] : spans_) { (void)id; if (span.end != 0U) result.push_back(span); }
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) { return left.id < right.id; });
    return result;
}

std::string TraceBuffer::render() const {
    std::ostringstream output;
    for (const auto& span : complete()) {
        output << span.id << ":parent=" << span.parent << ":" << span.name << ":duration=" << (span.end - span.start) << ":";
        std::vector<std::string> keys;
        for (const auto& [key, value] : span.attributes) { (void)value; keys.push_back(key); }
        std::sort(keys.begin(), keys.end());
        for (const auto& key : keys) output << key << "=" << span.attributes.at(key) << ",";
        output << "\n";
    }
    return output.str();
}

void TraceBuffer::clear() { tick_ = 0; next_id_ = 1; spans_.clear(); }

}  // namespace aegis::detail
