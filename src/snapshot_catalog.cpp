#include "internal.hpp"

namespace aegis::detail {

std::uint64_t SnapshotCatalog::record(std::uint32_t session_id, std::uint64_t generation,
                                      std::string label, std::span<const Byte> bytes) {
    SnapshotDescriptor descriptor;
    descriptor.id = next_id_++;
    descriptor.session_id = session_id;
    descriptor.generation = generation;
    descriptor.label = canonical_text(label.empty() ? "snapshot" : label);
    descriptor.bytes = bytes.size();
    descriptor.checksum = hex(encode_varint(checksum32(bytes)));
    descriptors_.push_back(std::move(descriptor));
    return descriptors_.back().id;
}

std::vector<SnapshotDescriptor> SnapshotCatalog::list(std::uint32_t session_id) const {
    std::vector<SnapshotDescriptor> result;
    for (const auto& descriptor : descriptors_) if (descriptor.session_id == session_id) result.push_back(descriptor);
    return result;
}

std::optional<SnapshotDescriptor> SnapshotCatalog::latest(std::uint32_t session_id) const {
    for (auto iterator = descriptors_.rbegin(); iterator != descriptors_.rend(); ++iterator) if (iterator->session_id == session_id) return *iterator;
    return std::nullopt;
}

std::size_t SnapshotCatalog::erase_before(std::uint64_t id) {
    const auto before = descriptors_.size();
    descriptors_.erase(std::remove_if(descriptors_.begin(), descriptors_.end(), [id](const auto& descriptor) { return descriptor.id < id; }), descriptors_.end());
    return before - descriptors_.size();
}

std::string SnapshotCatalog::report() const {
    std::ostringstream output;
    output << "snapshots=" << descriptors_.size() << ";";
    return output.str();
}

void SnapshotCatalog::clear() { next_id_ = 1; descriptors_.clear(); }

}  // namespace aegis::detail
