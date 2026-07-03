#include "internal.hpp"

namespace aegis::detail {

namespace {
std::string trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return std::string(value.substr(first, last - first + 1U));
}
}

std::vector<RuleAtom> RuleEvaluator::parse(std::string_view expression) const {
    std::vector<RuleAtom> atoms;
    std::size_t begin = 0;
    while (begin < expression.size()) {
        const auto end = expression.find(',', begin);
        const auto token = trim(expression.substr(begin, end == std::string_view::npos ? std::string_view::npos : end - begin));
        if (!token.empty()) {
            const auto equal = token.find('=');
            const auto not_equal = token.find("!=");
            RuleAtom atom;
            if (not_equal != std::string::npos) {
                atom.key = canonical_text(token.substr(0, not_equal));
                atom.op = "!=";
                atom.value = canonical_text(token.substr(not_equal + 2U));
            } else if (equal != std::string::npos) {
                atom.key = canonical_text(token.substr(0, equal));
                atom.op = "=";
                atom.value = canonical_text(token.substr(equal + 1U));
            } else {
                atom.key = canonical_text(token);
                atom.op = "present";
            }
            if (!atom.key.empty()) atoms.push_back(std::move(atom));
        }
        if (end == std::string_view::npos) break;
        begin = end + 1U;
    }
    return atoms;
}

RuleDecision RuleEvaluator::evaluate(const std::vector<RuleAtom>& atoms,
                                     const std::unordered_map<std::string, std::string>& facts) const {
    RuleDecision result;
    result.matched = true;
    for (const auto& atom : atoms) {
        const auto iterator = facts.find(atom.key);
        const auto actual = iterator == facts.end() ? std::string{} : canonical_text(iterator->second);
        bool match = false;
        if (atom.op == "present") match = iterator != facts.end() && !actual.empty();
        else if (atom.op == "=") match = actual == atom.value;
        else if (atom.op == "!=") match = iterator != facts.end() && actual != atom.value;
        if (match) {
            result.score += 10;
            result.evidence.push_back(atom.key + ":matched");
        } else {
            result.matched = false;
            result.score -= 15;
            result.evidence.push_back(atom.key + ":missed");
        }
    }
    if (atoms.empty()) result.evidence.push_back("no-atoms");
    return result;
}

RuleDecision RuleEvaluator::evaluate_policy(const CompiledPolicy* policy, const Session& session,
                                            std::string_view payload) const {
    std::unordered_map<std::string, std::string> facts;
    facts.emplace("session", std::to_string(session.id));
    facts.emplace("generation", std::to_string(session.generation));
    facts.emplace("payload", canonical_text(payload));
    facts.emplace("closed", session.closed ? "true" : "false");
    if (session.profile != nullptr) {
        facts.emplace("profile", std::to_string(session.profile->id));
        if (!session.profile->routes.empty()) facts.emplace("hop", session.profile->routes.front().next_hop);
    }
    std::vector<RuleAtom> atoms;
    RuleDecision advisory;
    if (policy != nullptr) {
        for (const auto& rule : policy->rules) {
            // Route labels are selectors rather than mandatory predicates. Only
            // explicit comparison expressions participate in admission.
            if (rule.match.find('=') != std::string::npos) {
                const auto parsed = parse(rule.match);
                atoms.insert(atoms.end(), parsed.begin(), parsed.end());
            } else if (!rule.match.empty()) {
                advisory.evidence.push_back("route-label:" + canonical_text(rule.match));
            }
        }
    }
    auto result = evaluate(atoms, facts);
    result.evidence.insert(result.evidence.end(), advisory.evidence.begin(), advisory.evidence.end());
    return result;
}

std::string RuleEvaluator::explain(const RuleDecision& decision) const {
    std::ostringstream output;
    output << (decision.matched ? "matched" : "not-matched") << ";score=" << decision.score << ";";
    for (const auto& evidence : decision.evidence) output << evidence << ";";
    return output.str();
}

}  // namespace aegis::detail
