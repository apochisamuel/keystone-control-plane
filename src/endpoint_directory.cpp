#include "internal.hpp"

namespace aegis::detail {

void EndpointDirectory::upsert(std::string name, std::string address, std::uint32_t weight) {
    if (!name.empty() && !address.empty()) endpoints_[std::move(name)] = {std::move(address), std::max(1U, weight)};
}

std::optional<std::string> EndpointDirectory::resolve(std::string_view name) const {
    const auto found = endpoints_.find(std::string(name)); return found == endpoints_.end() ? std::nullopt : std::optional<std::string>(found->second.address);
}

bool EndpointDirectory::remove(std::string_view name) { return endpoints_.erase(std::string(name)) != 0U; }

std::vector<std::string> EndpointDirectory::names() const { std::vector<std::string> out; for (const auto& [name, _] : endpoints_) out.push_back(name); std::sort(out.begin(), out.end()); return out; }

void EndpointDirectory::clear() { endpoints_.clear(); }

}  // namespace aegis::detail
