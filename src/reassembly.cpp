#include "internal.hpp"

namespace aegis::detail {

bool StreamReassembler::insert(FragmentKey key, std::uint32_t offset, std::span<const Byte> bytes,
                               bool terminal) {
    ++tick_;
    if (bytes.empty() && !terminal) return false;
    auto& assembly = assemblies_[key];
    assembly.tick = tick_;
    const std::uint64_t end = static_cast<std::uint64_t>(offset) + bytes.size();
    if (end > std::numeric_limits<std::uint32_t>::max()) return false;
    for (const auto& part : assembly.parts) {
        const auto part_end = static_cast<std::uint64_t>(part.offset) + part.bytes.size();
        const bool overlaps = !(end <= part.offset || offset >= part_end);
        if (overlaps) {
            if (part.offset == offset && part.bytes.size() == bytes.size() &&
                std::equal(part.bytes.begin(), part.bytes.end(), bytes.begin())) return true;
            return false;
        }
    }
    FragmentPart part;
    part.offset = offset;
    part.bytes.assign(bytes.begin(), bytes.end());
    part.terminal = terminal;
    assembly.parts.push_back(std::move(part));
    if (terminal) assembly.total = static_cast<std::uint32_t>(end);
    std::sort(assembly.parts.begin(), assembly.parts.end(), [](const auto& left, const auto& right) {
        return left.offset < right.offset;
    });
    return true;
}

std::optional<Bytes> StreamReassembler::take(FragmentKey key) {
    const auto iterator = assemblies_.find(key);
    if (iterator == assemblies_.end() || !iterator->second.total.has_value()) return std::nullopt;
    const auto total = *iterator->second.total;
    std::uint32_t cursor = 0;
    for (const auto& part : iterator->second.parts) {
        if (part.offset != cursor) return std::nullopt;
        cursor += static_cast<std::uint32_t>(part.bytes.size());
    }
    if (cursor != total) return std::nullopt;
    Bytes result;
    result.reserve(total);
    for (const auto& part : iterator->second.parts) result.insert(result.end(), part.bytes.begin(), part.bytes.end());
    assemblies_.erase(iterator);
    return result;
}

std::size_t StreamReassembler::discard_before(std::uint64_t tick_limit) {
    const auto before = assemblies_.size();
    for (auto iterator = assemblies_.begin(); iterator != assemblies_.end();) {
        if (iterator->second.tick < tick_limit) iterator = assemblies_.erase(iterator);
        else ++iterator;
    }
    return before - assemblies_.size();
}

std::size_t StreamReassembler::pending() const { return assemblies_.size(); }

std::string StreamReassembler::report() const {
    std::size_t parts = 0;
    std::size_t terminal = 0;
    for (const auto& [key, assembly] : assemblies_) {
        (void)key;
        parts += assembly.parts.size();
        if (assembly.total.has_value()) ++terminal;
    }
    std::ostringstream output;
    output << "reassembly.pending=" << assemblies_.size() << ";parts=" << parts << ";terminal=" << terminal << ";";
    return output.str();
}

void StreamReassembler::clear() {
    tick_ = 0;
    assemblies_.clear();
}

}  // namespace aegis::detail
