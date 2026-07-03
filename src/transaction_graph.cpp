#include "internal.hpp"

namespace aegis::detail {

std::uint64_t TransactionGraph::begin(std::string label){const auto id=next_id_++;nodes_[id]={id,std::move(label),false,{}};return id;}

bool TransactionGraph::link(std::uint64_t child,std::uint64_t parent){auto child_it=nodes_.find(child);const auto parent_it=nodes_.find(parent);if(child_it==nodes_.end()||parent_it==nodes_.end()||child==parent)return false;auto& parents=child_it->second.parents;if(std::find(parents.begin(),parents.end(),parent)==parents.end())parents.push_back(parent);return true;}

bool TransactionGraph::complete(std::uint64_t id){auto found=nodes_.find(id);if(found==nodes_.end())return false;for(auto parent:found->second.parents){const auto parent_it=nodes_.find(parent);if(parent_it!=nodes_.end()&&!parent_it->second.complete)return false;}found->second.complete=true;return true;}

std::vector<TransactionNode> TransactionGraph::pending() const {std::vector<TransactionNode> out;for(const auto&[_,node]:nodes_)if(!node.complete)out.push_back(node);std::sort(out.begin(),out.end(),[](const auto&a,const auto&b){return a.id<b.id;});return out;}

void TransactionGraph::clear(){nodes_.clear();next_id_=1U;}

}  // namespace aegis::detail
