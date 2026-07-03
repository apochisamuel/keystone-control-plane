#include "internal.hpp"

namespace aegis::detail {

void WeightedLoadBalancer::upsert(std::string name,std::uint32_t weight,bool enabled){if(name.empty())return;auto& endpoint=endpoints_[std::move(name)];endpoint.name=endpoint.name.empty()?"":endpoint.name;endpoint.weight=weight;endpoint.enabled=enabled;}

std::optional<std::string> WeightedLoadBalancer::choose(std::uint64_t entropy){std::vector<WeightedEndpoint*> candidates;std::uint64_t total{};for(auto&[name,endpoint]:endpoints_){if(endpoint.name.empty())endpoint.name=name;if(endpoint.enabled&&endpoint.weight!=0U){candidates.push_back(&endpoint);total+=endpoint.weight;}}if(total==0U)return std::nullopt;auto cursor=entropy%total;for(auto* endpoint:candidates){if(cursor<endpoint->weight){++endpoint->selected;return endpoint->name;}cursor-=endpoint->weight;}++candidates.back()->selected;return candidates.back()->name;}

std::vector<WeightedEndpoint> WeightedLoadBalancer::endpoints() const {std::vector<WeightedEndpoint> out;for(const auto&[name,endpoint]:endpoints_){auto copy=endpoint;if(copy.name.empty())copy.name=name;out.push_back(std::move(copy));}std::sort(out.begin(),out.end(),[](const auto&a,const auto&b){return a.name<b.name;});return out;}

void WeightedLoadBalancer::clear(){endpoints_.clear();}

}  // namespace aegis::detail
