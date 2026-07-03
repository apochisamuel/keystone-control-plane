#include "internal.hpp"

namespace aegis::detail {

void ServiceCatalog::register_service(std::string service, std::string version, std::vector<std::string> capabilities) { if(!service.empty()) services_[std::move(service)]={std::move(version),std::move(capabilities)}; }

std::optional<std::string> ServiceCatalog::locate(std::string_view capability) const { for(const auto& [name,entry] : services_) if(std::find(entry.capabilities.begin(),entry.capabilities.end(),capability)!=entry.capabilities.end()) return name; return std::nullopt; }

bool ServiceCatalog::remove(std::string_view service) { return services_.erase(std::string(service))!=0U; }

std::vector<std::string> ServiceCatalog::services() const { std::vector<std::string> out; for(const auto& [name,_]:services_)out.push_back(name);std::sort(out.begin(),out.end());return out; }

void ServiceCatalog::clear() { services_.clear(); }

}  // namespace aegis::detail
