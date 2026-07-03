#include "internal.hpp"

namespace aegis::detail {

std::vector<Frame> FrameCoalescer::coalesce(const std::vector<Frame>& frames) const {std::vector<Frame> out;for(const auto&frame:frames){if(!out.empty()&&out.back().session_id==frame.session_id&&out.back().operation==frame.operation&&out.back().flags==frame.flags){out.back().fields.insert(out.back().fields.end(),frame.fields.begin(),frame.fields.end());}else out.push_back(frame);}return out;}

std::size_t FrameCoalescer::count_payload(const std::vector<Frame>& frames) const {std::size_t total{};for(const auto&frame:frames)for(const auto&field:frame.fields)if(field.tag==static_cast<Byte>(FieldTag::Payload))total+=field.value.size();return total;}

}  // namespace aegis::detail
