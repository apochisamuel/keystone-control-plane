#include "internal.hpp"

namespace aegis::detail {

std::uint64_t AccountingWindow::post(std::uint32_t session_id,std::uint64_t units,std::uint64_t tick){const auto id=next_id_++;entries_.push_back({id,session_id,units,tick,false});return id;}

bool AccountingWindow::settle(std::uint64_t id){const auto found=std::find_if(entries_.begin(),entries_.end(),[id](const auto&entry){return entry.id==id;});if(found==entries_.end())return false;found->settled=true;return true;}

std::uint64_t AccountingWindow::total(std::uint32_t session_id,bool include_unsettled) const {std::uint64_t out{};for(const auto&entry:entries_)if(entry.session_id==session_id&&(include_unsettled||entry.settled))out+=entry.units;return out;}

void AccountingWindow::clear(){entries_.clear();next_id_=1U;}

}  // namespace aegis::detail
