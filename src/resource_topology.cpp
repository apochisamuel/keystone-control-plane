#include "internal.hpp"

namespace aegis::detail {

void ResourceTopology::connect(std::string left,std::string right){if(left.empty()||right.empty()||left==right)return;edges_[left].insert(right);edges_[right].insert(left);}

bool ResourceTopology::disconnect(std::string_view left,std::string_view right){const auto a=edges_.find(std::string(left));const auto b=edges_.find(std::string(right));if(a==edges_.end()||b==edges_.end())return false;const auto removed=a->second.erase(std::string(right))!=0U;b->second.erase(std::string(left));if(a->second.empty())edges_.erase(a);if(b->second.empty())edges_.erase(b);return removed;}

std::vector<std::string> ResourceTopology::neighbors(std::string_view node) const {const auto found=edges_.find(std::string(node));return found==edges_.end()?std::vector<std::string>{}:std::vector<std::string>(found->second.begin(),found->second.end());}

void ResourceTopology::clear(){edges_.clear();}

}  // namespace aegis::detail
