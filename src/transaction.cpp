#include "internal.hpp"

namespace aegis::detail {

std::uint64_t TransactionCoordinator::begin(std::uint32_t session_id, std::uint64_t generation, std::string name) {
    TransactionRecord record;
    record.id = next_id_++;
    record.session_id = session_id;
    record.generation = generation;
    record.name = canonical_text(name.empty() ? "transaction" : name);
    records_.emplace(record.id, std::move(record));
    return next_id_ - 1U;
}

bool TransactionCoordinator::add_step(std::uint64_t id, std::string participant, std::string operation) {
    auto iterator = records_.find(id);
    if (iterator == records_.end() || iterator->second.committed || iterator->second.rolled_back) return false;
    TransactionStep step;
    step.participant = canonical_text(participant);
    step.operation = canonical_text(operation);
    if (step.participant.empty() || step.operation.empty()) return false;
    iterator->second.steps.push_back(std::move(step));
    return true;
}

bool TransactionCoordinator::apply(std::uint64_t id, std::string participant, std::string result) {
    auto iterator = records_.find(id);
    if (iterator == records_.end() || iterator->second.committed || iterator->second.rolled_back) return false;
    const auto expected = canonical_text(participant);
    const auto step = std::find_if(iterator->second.steps.begin(), iterator->second.steps.end(), [&](const TransactionStep& item) {
        return item.participant == expected && !item.applied;
    });
    if (step == iterator->second.steps.end()) return false;
    step->applied = true;
    step->result = std::move(result);
    return true;
}

bool TransactionCoordinator::commit(std::uint64_t id) {
    auto iterator = records_.find(id);
    if (iterator == records_.end() || iterator->second.rolled_back || iterator->second.steps.empty()) return false;
    if (std::any_of(iterator->second.steps.begin(), iterator->second.steps.end(), [](const TransactionStep& step) { return !step.applied; })) return false;
    iterator->second.committed = true;
    return true;
}

bool TransactionCoordinator::rollback(std::uint64_t id, std::string reason) {
    auto iterator = records_.find(id);
    if (iterator == records_.end() || iterator->second.committed || iterator->second.rolled_back) return false;
    iterator->second.rolled_back = true;
    for (auto& step : iterator->second.steps) {
        if (step.applied) step.result += ":rolled-back:" + canonical_text(reason);
    }
    return true;
}

std::optional<TransactionRecord> TransactionCoordinator::record(std::uint64_t id) const {
    const auto iterator = records_.find(id);
    return iterator == records_.end() ? std::nullopt : std::optional<TransactionRecord>(iterator->second);
}

std::vector<TransactionRecord> TransactionCoordinator::history(std::uint32_t session_id) const {
    std::vector<TransactionRecord> result;
    for (const auto& [id, record] : records_) { (void)id; if (record.session_id == session_id) result.push_back(record); }
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) { return left.id < right.id; });
    return result;
}

std::string TransactionCoordinator::report() const {
    std::size_t committed = 0, rolled_back = 0;
    for (const auto& [id, record] : records_) { (void)id; committed += record.committed; rolled_back += record.rolled_back; }
    std::ostringstream output;
    output << "transactions.total=" << records_.size() << ";committed=" << committed << ";rolled-back=" << rolled_back << ";";
    return output.str();
}

void TransactionCoordinator::clear() { next_id_ = 1; records_.clear(); }

}  // namespace aegis::detail
