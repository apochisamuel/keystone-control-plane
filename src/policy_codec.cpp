#include "internal.hpp"

namespace aegis::detail {

namespace {
std::vector<std::string> split_columns(std::string_view value, char delimiter) {
    std::vector<std::string> result;
    std::size_t begin = 0;
    while (begin <= value.size()) {
        const auto end = value.find(delimiter, begin);
        result.emplace_back(value.substr(begin, end == std::string_view::npos ? std::string_view::npos : end - begin));
        if (end == std::string_view::npos) break;
        begin = end + 1U;
    }
    return result;
}
}

std::string PolicyCodec::encode(const PolicyCandidate& candidate) const {
    std::ostringstream output;
    output << candidate.session_id << '|'
           << candidate.profile_id << '|'
           << candidate.route_id << '|'
           << candidate.version << '|'
           << hex(Bytes(candidate.next_hop.begin(), candidate.next_hop.end())) << '|'
           << hex(Bytes(candidate.label.begin(), candidate.label.end())) << '|'
           << hex(candidate.extension) << '|'
           << (candidate.mirror_audit ? 1 : 0) << '|'
           << (candidate.route_mode ? 1 : 0);
    return output.str();
}

std::optional<PolicyCandidate> PolicyCodec::decode(std::string_view text) const {
    const auto columns = split_columns(text, '|');
    if (columns.size() != 9U) return std::nullopt;
    try {
        PolicyCandidate candidate;
        candidate.session_id = static_cast<std::uint32_t>(std::stoul(columns[0]));
        candidate.profile_id = static_cast<std::uint32_t>(std::stoul(columns[1]));
        candidate.route_id = static_cast<std::uint32_t>(std::stoul(columns[2]));
        candidate.version = std::stoull(columns[3]);
        const auto next_hop = unhex(columns[4]);
        const auto label = unhex(columns[5]);
        candidate.next_hop.assign(next_hop.begin(), next_hop.end());
        candidate.label.assign(label.begin(), label.end());
        candidate.extension = unhex(columns[6]);
        candidate.mirror_audit = columns[7] == "1";
        candidate.route_mode = columns[8] == "1";
        if (!validate_policy_candidate(candidate).empty()) return std::nullopt;
        return candidate;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::string PolicyCodec::fingerprint(const PolicyCandidate& candidate) const {
    return stable_digest(encode(candidate));
}

}  // namespace aegis::detail
