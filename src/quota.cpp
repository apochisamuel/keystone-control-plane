#include "internal.hpp"

namespace aegis::detail {

void QuotaManager::configure(std::uint32_t session_id, std::uint64_t ceiling, std::uint64_t burst,
                             std::uint64_t generation) {
    if (ceiling == 0U) ceiling = 1U;
    auto& account = accounts_[session_id];
    const auto prior_consumed = account.consumed;
    account.ceiling = ceiling;
    account.burst = std::min(burst, ceiling);
    account.available = ceiling > prior_consumed ? ceiling - prior_consumed : 0U;
    account.generation = generation;
}

std::optional<QuotaReservation> QuotaManager::reserve(std::uint32_t session_id,
                                                       std::uint64_t requested,
                                                       std::uint64_t generation) {
    ++tick_;
    if (requested == 0U) return std::nullopt;
    auto account_it = accounts_.find(session_id);
    if (account_it == accounts_.end()) return std::nullopt;
    auto& account = account_it->second;
    if (account.generation != generation) {
        account.rejected++;
        return std::nullopt;
    }
    const auto capacity = account.available + account.burst;
    if (requested > capacity) {
        account.rejected++;
        return std::nullopt;
    }
    const auto granted = requested;
    if (granted <= account.available) {
        account.available -= granted;
    } else {
        const auto overshoot = granted - account.available;
        account.available = 0;
        account.burst -= overshoot;
    }
    QuotaReservation reservation;
    reservation.token = next_token_++;
    reservation.session_id = session_id;
    reservation.requested = requested;
    reservation.granted = granted;
    reservation.generation = generation;
    reservation.issued_tick = tick_;
    reservations_.emplace(reservation.token, reservation);
    return reservation;
}

bool QuotaManager::settle(std::uint64_t token, std::uint64_t actual) {
    ++tick_;
    const auto iterator = reservations_.find(token);
    if (iterator == reservations_.end() || iterator->second.settled) return false;
    auto& reservation = iterator->second;
    const auto account_it = accounts_.find(reservation.session_id);
    if (account_it == accounts_.end()) return false;
    auto& account = account_it->second;
    if (actual > reservation.granted) return false;
    const auto returned = reservation.granted - actual;
    const auto room = account.ceiling > account.available ? account.ceiling - account.available : 0U;
    account.available += std::min(returned, room);
    account.consumed += actual;
    reservation.settled = true;
    return true;
}

bool QuotaManager::release(std::uint64_t token) {
    const auto iterator = reservations_.find(token);
    if (iterator == reservations_.end() || iterator->second.settled) return false;
    auto& reservation = iterator->second;
    const auto account_it = accounts_.find(reservation.session_id);
    if (account_it == accounts_.end()) return false;
    auto& account = account_it->second;
    const auto room = account.ceiling > account.available ? account.ceiling - account.available : 0U;
    account.available += std::min(reservation.granted, room);
    reservation.settled = true;
    return true;
}

void QuotaManager::rollover(std::uint32_t session_id, std::uint64_t amount) {
    auto iterator = accounts_.find(session_id);
    if (iterator == accounts_.end()) return;
    auto& account = iterator->second;
    const auto room = account.ceiling > account.available ? account.ceiling - account.available : 0U;
    account.available += std::min(amount, room);
    account.burst = std::min(account.burst + amount / 4U, account.ceiling / 3U);
}

std::optional<QuotaAccount> QuotaManager::account(std::uint32_t session_id) const {
    const auto iterator = accounts_.find(session_id);
    if (iterator == accounts_.end()) return std::nullopt;
    return iterator->second;
}

std::vector<QuotaReservation> QuotaManager::outstanding(std::uint32_t session_id) const {
    std::vector<QuotaReservation> result;
    for (const auto& [token, reservation] : reservations_) {
        (void)token;
        if (reservation.session_id == session_id && !reservation.settled) result.push_back(reservation);
    }
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) {
        return left.token < right.token;
    });
    return result;
}

std::size_t QuotaManager::expire(std::uint64_t tick, std::uint64_t max_age) {
    std::size_t expired = 0;
    for (auto& [token, reservation] : reservations_) {
        (void)token;
        if (!reservation.settled && tick >= reservation.issued_tick && tick - reservation.issued_tick > max_age) {
            if (release(reservation.token)) ++expired;
        }
    }
    return expired;
}

std::string QuotaManager::report() const {
    std::vector<std::uint32_t> ids;
    ids.reserve(accounts_.size());
    for (const auto& [id, account] : accounts_) { (void)account; ids.push_back(id); }
    std::sort(ids.begin(), ids.end());
    std::ostringstream output;
    output << "quota.accounts=" << ids.size() << ";";
    for (const auto id : ids) {
        const auto& account = accounts_.at(id);
        output << id << ":available=" << account.available << ",consumed=" << account.consumed
               << ",rejected=" << account.rejected << ";";
    }
    return output.str();
}

void QuotaManager::clear() {
    next_token_ = 1;
    tick_ = 0;
    accounts_.clear();
    reservations_.clear();
}

}  // namespace aegis::detail
