#include "internal.hpp"

namespace aegis::detail {

std::uint64_t EventJournal::append(std::uint32_t session_id, std::string component,
                                   std::string action, std::string payload) {
    JournalEntry entry;
    entry.sequence = next_sequence_++;
    entry.tick = ++tick_;
    entry.session_id = session_id;
    entry.component = canonical_text(component.empty() ? "engine" : component);
    entry.action = canonical_text(action.empty() ? "observe" : action);
    entry.payload = std::move(payload);
    entry.digest = stable_digest(entry.component + "|" + entry.action + "|" + entry.payload);
    entries_.push_back(std::move(entry));
    return entries_.back().sequence;
}

std::vector<JournalEntry> EventJournal::query(std::uint32_t session_id, std::string_view component) const {
    const auto expected = canonical_text(component);
    std::vector<JournalEntry> result;
    for (const auto& entry : entries_) {
        if (entry.session_id != session_id) continue;
        if (!expected.empty() && entry.component != expected) continue;
        result.push_back(entry);
    }
    return result;
}

std::vector<JournalEntry> EventJournal::tail(std::size_t limit) const {
    if (limit == 0U) return {};
    const auto begin = entries_.size() > limit ? entries_.size() - limit : 0U;
    return {entries_.begin() + static_cast<std::ptrdiff_t>(begin), entries_.end()};
}

std::vector<JournalEntry> EventJournal::since(std::uint64_t sequence) const {
    std::vector<JournalEntry> result;
    for (const auto& entry : entries_) if (entry.sequence > sequence) result.push_back(entry);
    return result;
}

std::size_t EventJournal::compact(std::uint64_t keep_from) {
    const auto before = entries_.size();
    entries_.erase(std::remove_if(entries_.begin(), entries_.end(), [keep_from](const JournalEntry& entry) {
        return entry.sequence < keep_from;
    }), entries_.end());
    return before - entries_.size();
}

std::string EventJournal::encode() const {
    std::ostringstream output;
    for (const auto& entry : entries_) {
        output << entry.sequence << '\t' << entry.tick << '\t' << entry.session_id << '\t'
               << entry.component << '\t' << entry.action << '\t' << hex(Bytes(entry.payload.begin(), entry.payload.end()))
               << '\t' << entry.digest << '\n';
    }
    return output.str();
}

bool EventJournal::decode_append(std::string_view text) {
    std::istringstream input{std::string(text)};
    std::string line;
    bool changed = false;
    while (std::getline(input, line)) {
        std::vector<std::string> columns;
        std::stringstream row(line);
        std::string column;
        while (std::getline(row, column, '\t')) columns.push_back(std::move(column));
        if (columns.size() != 7U) continue;
        try {
            JournalEntry entry;
            entry.sequence = std::stoull(columns[0]);
            entry.tick = std::stoull(columns[1]);
            entry.session_id = static_cast<std::uint32_t>(std::stoul(columns[2]));
            entry.component = columns[3];
            entry.action = columns[4];
            const auto raw = unhex(columns[5]);
            entry.payload.assign(raw.begin(), raw.end());
            entry.digest = columns[6];
            if (entry.digest != stable_digest(entry.component + "|" + entry.action + "|" + entry.payload)) continue;
            next_sequence_ = std::max(next_sequence_, entry.sequence + 1U);
            tick_ = std::max(tick_, entry.tick);
            entries_.push_back(std::move(entry));
            changed = true;
        } catch (const std::exception&) {
        }
    }
    return changed;
}

std::string EventJournal::report() const {
    std::unordered_map<std::string, std::size_t> by_component;
    for (const auto& entry : entries_) by_component[entry.component]++;
    std::vector<std::string> keys;
    for (const auto& [key, count] : by_component) { (void)count; keys.push_back(key); }
    std::sort(keys.begin(), keys.end());
    std::ostringstream output;
    output << "journal.entries=" << entries_.size() << ";";
    for (const auto& key : keys) output << key << "=" << by_component.at(key) << ";";
    return output.str();
}

void EventJournal::clear() {
    next_sequence_ = 1;
    tick_ = 0;
    entries_.clear();
}

}  // namespace aegis::detail
