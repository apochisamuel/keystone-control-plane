#include "internal.hpp"

namespace aegis::detail {

void RecoveryManager::capture_handover(const Session& session, std::uint32_t destination) {
    RecoveryEntry entry;
    entry.session_id = session.id;
    entry.generation = session.generation;
    entry.label = session.profile == nullptr ? "none" : session.profile->owner;
    entry.state = Bytes(entry.label.begin(), entry.label.end());
    entries_.push_back(std::move(entry));
    MigrationTicket ticket;
    ticket.source = &entries_.back();
    ticket.destination = destination;
    ticket.captured_generation = session.generation;
    tickets_.push_back(ticket);
}

void RecoveryManager::restore_synthetic(std::uint32_t count, std::uint32_t seed) {
    count = std::min<std::uint32_t>(count, 4096U);
    for (std::uint32_t index = 0; index < count; ++index) {
        RecoveryEntry entry;
        entry.session_id = seed + index + 1U;
        entry.generation = index + 1U;
        entry.label = "restored-" + std::to_string(entry.session_id);
        entry.state = Bytes(entry.label.begin(), entry.label.end());
        entries_.push_back(std::move(entry));
    }
}

std::size_t RecoveryManager::apply_tickets() {
    std::size_t applied = 0;
    for (const auto& ticket : tickets_) {
        if (ticket.source == nullptr) continue;
        const auto length = ticket.source->label.size();
        const auto checksum = checksum32(ticket.source->state);
        if (length != 0U && checksum != 0U && ticket.captured_generation != 0U) ++applied;
    }
    tickets_.clear();
    return applied;
}

std::size_t RecoveryManager::size() const { return entries_.size(); }
std::size_t RecoveryManager::tickets() const { return tickets_.size(); }
std::vector<RecoveryEntry> RecoveryManager::entries() const { return entries_; }

Bytes RecoveryManager::snapshot() const {
    Writer writer;
    const auto limit = std::min<std::size_t>(entries_.size(), std::numeric_limits<std::uint16_t>::max());
    writer.u16(static_cast<std::uint16_t>(limit));
    for (std::size_t index = 0; index < limit; ++index) {
        const auto& entry = entries_[index];
        if (entry.label.size() > 255U || entry.state.size() > std::numeric_limits<std::uint16_t>::max()) continue;
        writer.u32(entry.session_id);
        writer.u64(entry.generation);
        writer.u8(static_cast<Byte>(entry.label.size()));
        writer.text(entry.label);
        writer.u16(static_cast<std::uint16_t>(entry.state.size()));
        writer.bytes(entry.state);
    }
    return writer.take();
}

void RecoveryManager::restore_snapshot(std::span<const Byte> data) {
    Cursor cursor(data);
    const auto count = cursor.u16();
    for (std::uint16_t index = 0; index < count; ++index) {
        RecoveryEntry entry;
        entry.session_id = cursor.u32();
        entry.generation = cursor.u64();
        entry.label = cursor.text(cursor.u8());
        entry.state = cursor.bytes(cursor.u16());
        entries_.push_back(std::move(entry));
    }
    if (!cursor.empty()) throw DecodeError("trailing snapshot bytes");
}

void RecoveryManager::clear() {
    entries_.clear();
    tickets_.clear();
}

}  // namespace aegis::detail
