#include "internal.hpp"

namespace aegis::detail {

void BearerTransaction::stage(BearerChange change) {
    if (change.bearer_id == 0U) return;
    change.label = canonical_text(change.label);
    changes_.push_back(std::move(change));
}

std::vector<std::string> BearerTransaction::validate(const BearerStore& store) const {
    std::vector<std::string> errors;
    std::unordered_set<std::uint32_t> seen;
    for (const auto& change : changes_) {
        if (!seen.insert(change.bearer_id).second) errors.push_back("duplicate bearer change:" + std::to_string(change.bearer_id));
        if (!change.remove && change.qos == 0U) errors.push_back("zero qos:" + std::to_string(change.bearer_id));
        if (change.remove && !store.find(change.bearer_id).has_value()) errors.push_back("missing bearer removal:" + std::to_string(change.bearer_id));
    }
    return errors;
}

bool BearerTransaction::commit(BearerStore& store, std::uint64_t generation) {
    if (!validate(store).empty()) return false;
    for (const auto& change : changes_) {
        if (change.remove) {
            store.remove(change.bearer_id);
        } else {
            store.install(Bearer{change.bearer_id, change.qos, generation, true,
                                 change.label.empty() ? "managed" : change.label});
        }
    }
    changes_.clear();
    return true;
}

void BearerTransaction::rollback() { changes_.clear(); }
std::size_t BearerTransaction::pending() const { return changes_.size(); }

}  // namespace aegis::detail
