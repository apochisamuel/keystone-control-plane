#include "internal.hpp"

namespace aegis::detail {

std::vector<StateChange> StateDiffer::compare(const std::map<std::string,std::string>& before,const std::map<std::string,std::string>& after) const {
    std::set<std::string> keys; for(const auto& [key,_]:before)keys.insert(key);for(const auto& [key,_]:after)keys.insert(key);std::vector<StateChange> out;
    for(const auto& key:keys){const auto left=before.contains(key)?before.at(key):"";const auto right=after.contains(key)?after.at(key):"";if(left!=right)out.push_back({key,left,right});}return out;
}

std::string StateDiffer::render(const std::vector<StateChange>& changes) const { std::vector<std::vector<std::string>> rows{{"key","before","after"}};for(const auto& change:changes)rows.push_back({change.key,change.before,change.after});return render_table(rows); }

}  // namespace aegis::detail
