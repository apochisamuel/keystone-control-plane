#include "internal.hpp"

namespace aegis::detail {

void EventWindow::append(JournalEntry entry,std::size_t limit){entries_.push_back(std::move(entry));while(entries_.size()>limit)entries_.pop_front();}

std::vector<JournalEntry> EventWindow::between(std::uint64_t from,std::uint64_t to) const {std::vector<JournalEntry> out;for(const auto& entry:entries_)if(entry.sequence>=from&&entry.sequence<=to)out.push_back(entry);return out;}

std::size_t EventWindow::count(std::string_view component) const{return static_cast<std::size_t>(std::count_if(entries_.begin(),entries_.end(),[&](const auto&entry){return entry.component==component;}));}

void EventWindow::clear(){entries_.clear();}

}  // namespace aegis::detail
