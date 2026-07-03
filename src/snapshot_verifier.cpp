#include "internal.hpp"

namespace aegis::detail {

bool SnapshotVerifier::verify(const Checkpoint& checkpoint) const {return checkpoint.digest==stable_digest(checkpoint.reason+":"+checkpoint.state+":"+std::to_string(checkpoint.generation));}

std::vector<std::uint64_t> SnapshotVerifier::verify_all(const std::vector<Checkpoint>& checkpoints) const {std::vector<std::uint64_t> out;for(const auto&checkpoint:checkpoints)if(verify(checkpoint))out.push_back(checkpoint.id);return out;}

}  // namespace aegis::detail
