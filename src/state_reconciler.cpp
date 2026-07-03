#include "internal.hpp"

namespace aegis::detail {

std::vector<ReconciliationChange> StateReconciler::reconcile(const std::map<std::string,std::string>& desired,const std::map<std::string,std::string>& observed) const {std::set<std::string> keys;for(const auto&[key,_]:desired)keys.insert(key);for(const auto&[key,_]:observed)keys.insert(key);std::vector<ReconciliationChange> changes;for(const auto&key:keys){const auto want=desired.contains(key)?desired.at(key):"";const auto have=observed.contains(key)?observed.at(key):"";if(want!=have)changes.push_back({key,have,want});}return changes;}

std::map<std::string,std::string> StateReconciler::apply(const std::map<std::string,std::string>& observed,const std::vector<ReconciliationChange>& changes) const {auto out=observed;for(const auto&change:changes){if(change.after.empty())out.erase(change.key);else out[change.key]=change.after;}return out;}

}  // namespace aegis::detail
