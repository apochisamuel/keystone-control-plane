#include "internal.hpp"

namespace aegis::detail {

std::uint64_t VersionedSnapshotStore::capture(std::uint32_t session_id, std::uint64_t generation,
                                              std::string kind, std::string_view state,
                                              std::uint32_t version) {
    if (kind.empty()) kind = "session";
    VersionedSnapshot snapshot;
    snapshot.id = next_id_++;
    snapshot.session_id = session_id;
    snapshot.generation = generation;
    snapshot.version = std::max(1U, version);
    snapshot.kind = std::move(kind);
    snapshot.payload.assign(state.begin(), state.end());
    snapshot.digest = stable_digest(snapshot.kind + ":" + std::to_string(snapshot.version) + ":" +
                                    std::to_string(snapshot.generation) + ":" + std::string(state));
    snapshots_[session_id].push_back(snapshot);
    return snapshot.id;
}

std::optional<VersionedSnapshot> VersionedSnapshotStore::latest(std::uint32_t session_id,
                                                                 std::string_view kind) const {
    const auto found = snapshots_.find(session_id);
    if (found == snapshots_.end()) return std::nullopt;
    for (auto iterator = found->second.rbegin(); iterator != found->second.rend(); ++iterator) {
        if (kind.empty() || iterator->kind == kind) return *iterator;
    }
    return std::nullopt;
}

std::vector<VersionedSnapshot> VersionedSnapshotStore::history(std::uint32_t session_id) const {
    const auto found = snapshots_.find(session_id);
    return found == snapshots_.end() ? std::vector<VersionedSnapshot>{} : found->second;
}

bool VersionedSnapshotStore::verify(const VersionedSnapshot& snapshot) const {
    const std::string state(snapshot.payload.begin(), snapshot.payload.end());
    return snapshot.id != 0U && snapshot.session_id != 0U && snapshot.version != 0U &&
           snapshot.digest == stable_digest(snapshot.kind + ":" + std::to_string(snapshot.version) + ":" +
                                            std::to_string(snapshot.generation) + ":" + state);
}

std::size_t VersionedSnapshotStore::prune(std::uint32_t session_id, std::size_t keep) {
    auto found = snapshots_.find(session_id);
    if (found == snapshots_.end() || found->second.size() <= keep) return 0U;
    const auto removed = found->second.size() - keep;
    found->second.erase(found->second.begin(), found->second.begin() + static_cast<std::ptrdiff_t>(removed));
    if (found->second.empty()) snapshots_.erase(found);
    return removed;
}

std::string VersionedSnapshotStore::report() const {
    std::map<std::string, std::size_t> kinds;
    std::size_t invalid{};
    for (const auto& [_, history] : snapshots_) {
        for (const auto& snapshot : history) {
            ++kinds[snapshot.kind];
            if (!verify(snapshot)) ++invalid;
        }
    }
    std::vector<std::vector<std::string>> rows{{"metric", "value"}};
    rows.push_back({"sessions", std::to_string(snapshots_.size())});
    rows.push_back({"invalid", std::to_string(invalid)});
    for (const auto& [kind, count] : kinds) rows.push_back({"kind." + kind, std::to_string(count)});
    return render_table(rows);
}

void VersionedSnapshotStore::clear() {
    snapshots_.clear();
    next_id_ = 1U;
}

}  // namespace aegis::detail
