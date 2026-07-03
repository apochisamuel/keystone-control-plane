#include "internal.hpp"

namespace aegis::detail {

bool CommandDeduplicator::accept(std::uint32_t session_id,std::string command,std::uint64_t tick){const auto key=std::to_string(session_id)+":"+command;if(seen_.contains(key))return false;seen_[key]=tick;return true;}

std::size_t CommandDeduplicator::size() const{return seen_.size();}

void CommandDeduplicator::clear(){seen_.clear();}

}  // namespace aegis::detail
