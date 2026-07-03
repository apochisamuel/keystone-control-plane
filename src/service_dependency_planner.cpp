#include "internal.hpp"

namespace aegis::detail {

void ServiceDependencyPlanner::register_service(std::string service,std::vector<std::string> dependencies){if(!service.empty())services_[std::move(service)]=std::move(dependencies);}

std::vector<std::string> ServiceDependencyPlanner::startup_plan() const {std::vector<std::string> out;std::set<std::string> visiting,complete;std::function<void(const std::string&)> visit=[&](const std::string&service){if(complete.contains(service)||visiting.contains(service))return;visiting.insert(service);if(const auto found=services_.find(service);found!=services_.end())for(const auto&dependency:found->second)visit(dependency);visiting.erase(service);complete.insert(service);out.push_back(service);};for(const auto&[service,_]:services_)visit(service);return out;}

std::vector<std::string> ServiceDependencyPlanner::validate() const {std::vector<std::string> errors;for(const auto&[service,dependencies]:services_)for(const auto&dependency:dependencies)if(!services_.contains(dependency))errors.push_back(service+" requires unknown "+dependency);return errors;}

void ServiceDependencyPlanner::clear(){services_.clear();}

}  // namespace aegis::detail
