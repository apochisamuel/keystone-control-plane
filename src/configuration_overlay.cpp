#include "internal.hpp"

namespace aegis::detail {

void ConfigurationOverlay::set(std::string layer,std::map<std::string,std::string> values){if(!layer.empty())layers_[std::move(layer)]=std::move(values);}

std::optional<std::string> ConfigurationOverlay::resolve(std::string_view key) const {std::optional<std::string> value;for(const auto&[_,layer]:layers_)if(const auto found=layer.find(std::string(key));found!=layer.end())value=found->second;return value;}

std::map<std::string,std::string> ConfigurationOverlay::merged() const {std::map<std::string,std::string> out;for(const auto&[_,layer]:layers_)for(const auto&[key,value]:layer)out[key]=value;return out;}

void ConfigurationOverlay::clear(){layers_.clear();}

}  // namespace aegis::detail
