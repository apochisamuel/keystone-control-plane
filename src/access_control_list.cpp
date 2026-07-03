#include "internal.hpp"

namespace aegis::detail {

void AccessControlList::grant(std::uint32_t subject, std::string capability) {
    if (!capability.empty()) grants_[subject].insert(std::move(capability));
}

bool AccessControlList::revoke(std::uint32_t subject, std::string_view capability) {
    const auto found = grants_.find(subject);
    if (found == grants_.end()) return false;
    const auto removed = found->second.erase(std::string(capability)) != 0U;
    if (found->second.empty()) grants_.erase(found);
    return removed;
}

bool AccessControlList::allows(std::uint32_t subject, std::string_view capability) const {
    const auto found = grants_.find(subject);
    return found != grants_.end() && found->second.contains(std::string(capability));
}

std::vector<std::string> AccessControlList::capabilities(std::uint32_t subject) const {
    const auto found = grants_.find(subject);
    if (found == grants_.end()) return {};
    return {found->second.begin(), found->second.end()};
}

void AccessControlList::clear() { grants_.clear(); }

}  // namespace aegis::detail
