#include "internal.hpp"

namespace aegis::detail {

std::string PolicyTextParser::trim(std::string_view text) const {
    std::size_t first{};
    while (first < text.size() && std::isspace(static_cast<unsigned char>(text[first])) != 0) ++first;
    std::size_t last = text.size();
    while (last > first && std::isspace(static_cast<unsigned char>(text[last - 1U])) != 0) --last;
    return std::string(text.substr(first, last - first));
}

std::optional<std::uint32_t> PolicyTextParser::number(std::string_view text) const {
    const auto value = trim(text);
    if (value.empty()) return std::nullopt;
    std::uint64_t parsed{};
    for (const auto character : value) {
        if (character < '0' || character > '9') return std::nullopt;
        parsed = parsed * 10U + static_cast<std::uint64_t>(character - '0');
        if (parsed > std::numeric_limits<std::uint32_t>::max()) return std::nullopt;
    }
    return static_cast<std::uint32_t>(parsed);
}

PolicyTextDocument PolicyTextParser::parse(std::string_view text) const {
    PolicyTextDocument document;
    std::size_t offset{};
    std::size_t line_number{1U};
    while (offset <= text.size()) {
        const auto end = text.find('\n', offset);
        const auto line = trim(text.substr(offset, end == std::string_view::npos ? text.size() - offset : end - offset));
        if (!line.empty() && line.front() != '#') {
            const auto equals = line.find('=');
            if (equals == std::string::npos) {
                document.warnings.push_back("line " + std::to_string(line_number) + " lacks equals sign");
            } else {
                const auto key = canonical_text(trim(line.substr(0U, equals)));
                const auto value = trim(line.substr(equals + 1U));
                if (key.empty()) document.warnings.push_back("line " + std::to_string(line_number) + " has empty key");
                else if (document.values.contains(key)) document.warnings.push_back("line " + std::to_string(line_number) + " overrides " + key);
                document.values[key] = value;
            }
        }
        if (end == std::string_view::npos) break;
        offset = end + 1U;
        ++line_number;
    }
    return document;
}

std::vector<std::string> PolicyTextParser::validate(const PolicyTextDocument& document) const {
    std::vector<std::string> errors;
    for (const auto& required : {"profile", "route", "version", "next-hop"}) {
        if (!document.values.contains(required)) errors.push_back("missing " + std::string(required));
    }
    for (const auto& numeric : {"profile", "route", "version"}) {
        if (const auto found = document.values.find(numeric); found != document.values.end() && !number(found->second)) {
            errors.push_back("invalid numeric value for " + std::string(numeric));
        }
    }
    if (const auto found = document.values.find("next-hop"); found != document.values.end() && found->second.empty()) errors.push_back("next-hop is empty");
    return errors;
}

std::optional<PolicyCandidate> PolicyTextParser::candidate(std::uint32_t session_id,
                                                           const PolicyTextDocument& document) const {
    if (!validate(document).empty()) return std::nullopt;
    PolicyCandidate candidate;
    candidate.session_id = session_id;
    candidate.profile_id = *number(document.values.at("profile"));
    candidate.route_id = *number(document.values.at("route"));
    candidate.version = *number(document.values.at("version"));
    candidate.next_hop = document.values.at("next-hop");
    candidate.label = document.values.contains("label") ? document.values.at("label") : candidate.next_hop;
    candidate.route_mode = document.values.contains("route-mode") && canonical_text(document.values.at("route-mode")) == "true";
    candidate.mirror_audit = document.values.contains("mirror-audit") && canonical_text(document.values.at("mirror-audit")) == "true";
    if (document.values.contains("extension")) candidate.extension = unhex(document.values.at("extension"));
    return candidate;
}

std::string PolicyTextParser::render(const PolicyTextDocument& document) const {
    std::vector<std::vector<std::string>> rows{{"key", "value"}};
    for (const auto& [key, value] : document.values) rows.push_back({key, value});
    if (!document.warnings.empty()) {
        rows.push_back({"warnings", join_labels(document.warnings, ";")});
    }
    return render_table(rows);
}

}  // namespace aegis::detail
