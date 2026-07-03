#include "internal.hpp"

namespace aegis::detail {

void ProtocolExtensionRegistry::register_schema(ExtensionSchema schema){if(schema.minimum<=schema.maximum)schemas_[schema.tag]=std::move(schema);}

bool ProtocolExtensionRegistry::accepts(std::uint8_t tag,std::size_t length) const {const auto found=schemas_.find(tag);return found!=schemas_.end()&&length>=found->second.minimum&&length<=found->second.maximum;}

std::vector<ExtensionSchema> ProtocolExtensionRegistry::schemas() const {std::vector<ExtensionSchema> out;for(const auto&[_,schema]:schemas_)out.push_back(schema);return out;}

void ProtocolExtensionRegistry::clear(){schemas_.clear();}

}  // namespace aegis::detail
