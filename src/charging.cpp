#include "internal.hpp"

namespace aegis::detail {

namespace {
std::uint64_t category_rate(std::string_view category) {
    if (category == "handover") return 31U;
    if (category == "priority") return 17U;
    if (category == "recovery") return 9U;
    if (category == "audit") return 3U;
    return 11U;
}
}

std::uint64_t ChargingLedger::post(std::uint32_t session_id, std::uint64_t generation,
                                   std::uint64_t units, std::string category, std::string reference) {
    if (units == 0U) return 0U;
    const auto normalized = canonical_text(reference);
    if (!normalized.empty()) {
        const auto duplicate = reference_index_.find(normalized);
        if (duplicate != reference_index_.end()) return duplicate->second;
    }
    ChargeLine line;
    line.id = next_line_++;
    line.session_id = session_id;
    line.generation = generation;
    line.units = units;
    line.category = canonical_text(category.empty() ? "data" : category);
    line.reference = normalized;
    const auto rate = category_rate(line.category);
    if (units > std::numeric_limits<std::uint64_t>::max() / rate) {
        line.amount_micros = std::numeric_limits<std::uint64_t>::max();
    } else {
        line.amount_micros = units * rate;
    }
    lines_.push_back(line);
    if (!normalized.empty()) reference_index_.emplace(normalized, line.id);
    return line.id;
}

bool ChargingLedger::reverse(std::uint64_t line_id, std::string reason) {
    for (auto& line : lines_) {
        if (line.id != line_id || line.reversed) continue;
        line.reversed = true;
        line.reference += ":reversed:" + canonical_text(reason);
        return true;
    }
    return false;
}

std::uint64_t ChargingLedger::total(std::uint32_t session_id) const {
    std::uint64_t total_amount = 0;
    for (const auto& line : lines_) {
        if (line.session_id != session_id || line.reversed) continue;
        const auto room = std::numeric_limits<std::uint64_t>::max() - total_amount;
        total_amount += std::min(room, line.amount_micros);
    }
    return total_amount;
}

std::uint64_t ChargingLedger::total_by_category(std::string_view category) const {
    const auto expected = canonical_text(category);
    std::uint64_t total_amount = 0;
    for (const auto& line : lines_) {
        if (line.reversed || line.category != expected) continue;
        const auto room = std::numeric_limits<std::uint64_t>::max() - total_amount;
        total_amount += std::min(room, line.amount_micros);
    }
    return total_amount;
}

std::vector<ChargeLine> ChargingLedger::lines(std::uint32_t session_id) const {
    std::vector<ChargeLine> result;
    for (const auto& line : lines_) if (line.session_id == session_id) result.push_back(line);
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) { return left.id < right.id; });
    return result;
}

std::vector<ChargeLine> ChargingLedger::reconcile(std::uint32_t session_id, std::uint64_t generation) const {
    std::vector<ChargeLine> result;
    for (const auto& line : lines_) {
        if (line.session_id == session_id && line.generation == generation && !line.reversed) result.push_back(line);
    }
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) {
        if (left.category != right.category) return left.category < right.category;
        return left.id < right.id;
    });
    return result;
}

std::string ChargingLedger::receipt(std::uint32_t session_id) const {
    const auto session_lines = lines(session_id);
    std::ostringstream output;
    output << "session=" << session_id << ";lines=" << session_lines.size() << ";total=" << total(session_id) << ";";
    for (const auto& line : session_lines) {
        output << line.id << ":" << line.category << ":" << line.units << ":" << line.amount_micros
               << (line.reversed ? ":reversed" : ":posted") << ";";
    }
    return output.str();
}

void ChargingLedger::clear() {
    next_line_ = 1;
    lines_.clear();
    reference_index_.clear();
}

}  // namespace aegis::detail
