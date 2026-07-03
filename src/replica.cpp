#include "internal.hpp"

namespace aegis::detail {

namespace {
std::string replica_key(std::uint32_t session_id, std::string_view shard) {
    return std::to_string(session_id) + ":" + canonical_text(shard);
}
}

void StateReplica::write(std::uint32_t session_id, std::uint64_t generation, std::string shard, std::string state) {
    ReplicaRecord record;
    record.session_id = session_id;
    record.generation = generation;
    record.shard = canonical_text(shard.empty() ? "primary" : shard);
    record.state = std::move(state);
    record.digest = stable_digest(std::to_string(session_id) + "|" + std::to_string(generation) + "|" + record.shard + "|" + record.state);
    records_[replica_key(session_id, record.shard)] = std::move(record);
}

std::optional<ReplicaRecord> StateReplica::read(std::uint32_t session_id, std::string_view shard) const {
    const auto iterator = records_.find(replica_key(session_id, shard));
    return iterator == records_.end() ? std::nullopt : std::optional<ReplicaRecord>(iterator->second);
}

std::vector<ReplicaRecord> StateReplica::replicas(std::uint32_t session_id) const {
    std::vector<ReplicaRecord> result;
    for (const auto& [key, record] : records_) { (void)key; if (record.session_id == session_id) result.push_back(record); }
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) { return left.shard < right.shard; });
    return result;
}

std::vector<std::string> StateReplica::validate(std::uint32_t session_id, std::uint64_t expected_generation) const {
    std::vector<std::string> errors;
    for (const auto& record : replicas(session_id)) {
        if (record.generation != expected_generation) errors.push_back(record.shard + ":generation-mismatch");
        if (record.digest != stable_digest(std::to_string(record.session_id) + "|" + std::to_string(record.generation) + "|" + record.shard + "|" + record.state)) {
            errors.push_back(record.shard + ":digest-mismatch");
        }
    }
    return errors;
}

std::string StateReplica::report() const {
    std::ostringstream output;
    output << "replicas=" << records_.size() << ";";
    return output.str();
}

void StateReplica::clear() { records_.clear(); }

}  // namespace aegis::detail
