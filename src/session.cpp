#include "internal.hpp"

namespace aegis::detail {

namespace {
std::unique_ptr<Profile> make_profile(std::uint32_t profile_id, std::uint64_t generation,
                                      std::string next_hop, std::uint32_t route_id) {
    auto profile = std::make_unique<Profile>();
    profile->id = profile_id;
    profile->generation = generation;
    profile->owner = "profile-" + std::to_string(profile_id);
    Route route;
    route.id = route_id == 0 ? profile_id : route_id;
    route.next_hop = canonical_text(next_hop.empty() ? "default-hop" : next_hop);
    route.weight = static_cast<std::uint32_t>((profile_id % 17U) + 1U);
    route.profile_generation = generation;
    route.labels = split_labels(next_hop);
    if (route.labels.empty()) route.labels.push_back("default");
    profile->routes.push_back(std::move(route));
    profile->attributes.push_back("generation=" + std::to_string(generation));
    return profile;
}
}

Session& SessionStore::open(std::uint32_t session_id, std::uint32_t profile_id, std::string next_hop, std::uint32_t route_id) {
    auto iterator = sessions_.find(session_id);
    if (iterator != sessions_.end()) return *iterator->second;
    auto session = std::make_unique<Session>();
    session->id = session_id;
    session->generation = 1;
    session->subscriber.value = session_id;
    session->subscriber.normalized = "subscriber-" + std::to_string(session_id);
    session->profile = make_profile(profile_id, session->generation, std::move(next_hop), route_id);
    session->quota = 1000;
    auto [inserted, accepted] = sessions_.emplace(session_id, std::move(session));
    (void)accepted;
    return *inserted->second;
}

Session* SessionStore::find(std::uint32_t session_id) {
    const auto iterator = sessions_.find(session_id);
    return iterator == sessions_.end() ? nullptr : iterator->second.get();
}

const Session* SessionStore::find(std::uint32_t session_id) const {
    const auto iterator = sessions_.find(session_id);
    return iterator == sessions_.end() ? nullptr : iterator->second.get();
}

void SessionStore::stage_profile(std::uint32_t session_id, std::uint32_t profile_id, std::string next_hop, std::uint32_t route_id) {
    auto* session = find(session_id);
    if (session == nullptr || session->closed) throw DecodeError("cannot stage unknown session");
    const auto next_generation = session->generation + 1U;
    session->staged_profile = make_profile(profile_id, next_generation, std::move(next_hop), route_id);
}

void SessionStore::commit_profile(std::uint32_t session_id) {
    auto* session = find(session_id);
    if (session == nullptr || session->staged_profile == nullptr) throw DecodeError("cannot commit profile");
    auto previous = std::move(session->profile);
    session->profile = std::move(session->staged_profile);
    session->generation = session->profile->generation;
    session->profile->attributes.push_back("identity-preserved=" + std::to_string(session_id));
}

void SessionStore::close(std::uint32_t session_id) {
    auto* session = find(session_id);
    if (session == nullptr) return;
    session->closed = true;
    session->tunnels = TunnelStore{};
    session->bearers = BearerStore{};
}

std::vector<Session*> SessionStore::all() {
    std::vector<Session*> result;
    result.reserve(sessions_.size());
    for (auto& [id, session] : sessions_) {
        (void)id;
        result.push_back(session.get());
    }
    std::sort(result.begin(), result.end(), [](const Session* left, const Session* right) { return left->id < right->id; });
    return result;
}

std::vector<const Session*> SessionStore::all() const {
    std::vector<const Session*> result;
    result.reserve(sessions_.size());
    for (const auto& [id, session] : sessions_) {
        (void)id;
        result.push_back(session.get());
    }
    std::sort(result.begin(), result.end(), [](const Session* left, const Session* right) { return left->id < right->id; });
    return result;
}

std::size_t SessionStore::size() const { return sessions_.size(); }
void SessionStore::clear() { sessions_.clear(); }

}  // namespace aegis::detail
