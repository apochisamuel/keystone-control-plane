#include "internal.hpp"

namespace aegis::detail {

void FailureDomainTable::assign(std::uint32_t session_id, std::string domain) { if (!domain.empty()) { assignments_[session_id] = domain; health_.try_emplace(std::move(domain), true); } }

std::vector<std::uint32_t> FailureDomainTable::members(std::string_view domain) const { std::vector<std::uint32_t> out; for (const auto& [session, name] : assignments_) if (name == domain) out.push_back(session); std::sort(out.begin(), out.end()); return out; }

void FailureDomainTable::mark(std::string domain, bool healthy) { if (!domain.empty()) health_[std::move(domain)] = healthy; }

bool FailureDomainTable::healthy(std::string_view domain) const { const auto found = health_.find(std::string(domain)); return found != health_.end() && found->second; }

void FailureDomainTable::clear() { assignments_.clear(); health_.clear(); }

}  // namespace aegis::detail
