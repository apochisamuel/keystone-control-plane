#include "internal.hpp"

namespace aegis::detail {

ReconciliationItem ChargeReconciler::reconcile(std::uint32_t session_id, const UsageAggregator& usage,
                                                const ChargingLedger& ledger) const {
    ReconciliationItem item;
    item.session_id = session_id;
    if (const auto bucket = usage.bucket(session_id); bucket.has_value()) item.expected_units = bucket->payload_bytes + bucket->decisions;
    for (const auto& line : ledger.lines(session_id)) {
        if (line.reversed) continue;
        item.charged_units += line.units;
        item.related_lines.push_back(line.id);
    }
    if (item.charged_units > item.expected_units) {
        const auto delta = item.charged_units - item.expected_units;
        item.delta = delta > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) ? std::numeric_limits<std::int64_t>::max() : static_cast<std::int64_t>(delta);
    } else {
        const auto delta = item.expected_units - item.charged_units;
        item.delta = delta > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) ? std::numeric_limits<std::int64_t>::min() : -static_cast<std::int64_t>(delta);
    }
    return item;
}

std::string ChargeReconciler::explain(const ReconciliationItem& item) const {
    std::ostringstream output;
    output << "session=" << item.session_id << ";expected=" << item.expected_units << ";charged=" << item.charged_units << ";delta=" << item.delta << ";lines=";
    for (const auto id : item.related_lines) output << id << ",";
    return output.str();
}

}  // namespace aegis::detail
