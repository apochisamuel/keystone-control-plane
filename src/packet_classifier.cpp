#include "internal.hpp"

namespace aegis::detail {

void PacketClassifier::set_weight(std::string label,std::uint32_t weight_value){if(!label.empty())weights_[std::move(label)]=weight_value;}

std::string PacketClassifier::classify(const Frame& frame) const {const auto op=describe_operation(frame.operation);if(const auto found=weights_.find(op);found!=weights_.end()&&found->second>=50U)return "priority:"+op;if(frame.operation==Operation::Data)return "data";if(frame.operation==Operation::Heartbeat)return "control";return "state";}

std::uint32_t PacketClassifier::weight(std::string_view label) const {const auto found=weights_.find(std::string(label));return found==weights_.end()?0U:found->second;}

void PacketClassifier::clear(){weights_.clear();}

}  // namespace aegis::detail
