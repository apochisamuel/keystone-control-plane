#include "internal.hpp"

namespace aegis::detail {

std::vector<JournalEntry> JournalCompactor::compact(const std::vector<JournalEntry>& entries,std::size_t limit) const {std::map<std::pair<std::uint32_t,std::string>,JournalEntry> latest;for(const auto&entry:entries){const auto key=std::make_pair(entry.session_id,entry.component);const auto found=latest.find(key);if(found==latest.end()||entry.sequence>found->second.sequence)latest[key]=entry;}std::vector<JournalEntry> out;for(auto&[_,entry]:latest)out.push_back(std::move(entry));std::sort(out.begin(),out.end(),[](const auto&a,const auto&b){return a.sequence<b.sequence;});if(out.size()>limit)out.erase(out.begin(),out.begin()+static_cast<std::ptrdiff_t>(out.size()-limit));return out;}

std::string JournalCompactor::digest(const std::vector<JournalEntry>& entries) const {std::string text;for(const auto&entry:entries)text+=std::to_string(entry.sequence)+":"+entry.digest+";";return stable_digest(text);}

}  // namespace aegis::detail
