#include "internal.hpp"

namespace aegis::detail {

std::string serialize_session(const Session& session) {
    std::ostringstream output;
    output << session.id << '|' << session.generation << '|' << session.quota << '|'
           << session.closed << '|' << session.subscriber.normalized << '|';
    if (session.profile == nullptr) return output.str();
    output << session.profile->id << '|' << session.profile->owner << '|';
    for (std::size_t index = 0; index < session.profile->routes.size(); ++index) {
        const auto& route = session.profile->routes[index];
        if (index != 0) output << ';';
        output << route.id << ',' << route.weight << ',' << route.next_hop << ',' << join_labels(route.labels, ",");
    }
    return output.str();
}

std::optional<Session> deserialize_session(std::string_view text) {
    std::vector<std::string> pieces;
    std::string current;
    for (const char ch : text) {
        if (ch == '|') { pieces.push_back(current); current.clear(); }
        else current.push_back(ch);
    }
    pieces.push_back(current);
    if (pieces.size() < 8U) return std::nullopt;
    try {
        Session session;
        session.id = static_cast<std::uint32_t>(std::stoul(pieces[0]));
        session.generation = std::stoull(pieces[1]);
        session.quota = static_cast<std::uint32_t>(std::stoul(pieces[2]));
        session.closed = std::stoul(pieces[3]) != 0U;
        session.subscriber.value = session.id;
        session.subscriber.normalized = pieces[4];
        auto profile = std::make_unique<Profile>();
        profile->id = static_cast<std::uint32_t>(std::stoul(pieces[5]));
        profile->generation = session.generation;
        profile->owner = pieces[6];
        std::stringstream routes(pieces[7]);
        std::string row;
        while (std::getline(routes, row, ';')) {
            std::stringstream fields(row);
            std::string value;
            std::vector<std::string> values;
            while (std::getline(fields, value, ',')) values.push_back(value);
            if (values.size() < 3U) continue;
            Route route;
            route.id = static_cast<std::uint32_t>(std::stoul(values[0]));
            route.weight = static_cast<std::uint32_t>(std::stoul(values[1]));
            route.next_hop = values[2];
            if (values.size() > 3U) route.labels = split_labels(values[3]);
            route.profile_generation = session.generation;
            profile->routes.push_back(std::move(route));
        }
        session.profile = std::move(profile);
        return session;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

}  // namespace aegis::detail
