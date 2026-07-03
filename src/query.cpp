#include "internal.hpp"

namespace aegis::detail {

std::string query_session(const Session& session) {
    std::ostringstream output;
    output << "session=" << session.id << '\n';
    output << "subscriber=" << session.subscriber.normalized << '\n';
    output << "generation=" << session.generation << '\n';
    output << "closed=" << session.closed << '\n';
    output << "quota=" << session.quota << '\n';
    output << "accepted_frames=" << session.accepted_frames << '\n';
    if (session.profile != nullptr) {
        output << "profile=" << session.profile->id << ':' << session.profile->owner << '\n';
        for (const auto& route : session.profile->routes) {
            output << "route=" << route.id << ':' << route.next_hop << ':' << route.weight << ':'
                   << join_labels(route.labels, ",") << '\n';
        }
    }
    for (const auto& bearer : session.bearers.active()) {
        output << "bearer=" << bearer.id << ':' << bearer.qos << ':' << bearer.label << '\n';
    }
    for (const auto& tunnel : session.tunnels.bound()) {
        output << "tunnel=" << tunnel.id << ':' << tunnel.peer << ':' << tunnel.endpoint << '\n';
    }
    return output.str();
}

}  // namespace aegis::detail
