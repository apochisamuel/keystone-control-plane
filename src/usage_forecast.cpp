#include "internal.hpp"

namespace aegis::detail {

void UsageForecast::observe(std::uint32_t session_id,std::uint64_t units){auto& values=samples_[session_id];values.push_back(units);while(values.size()>12U)values.pop_front();}

std::uint64_t UsageForecast::predict(std::uint32_t session_id) const {const auto found=samples_.find(session_id);if(found==samples_.end()||found->second.empty())return 0U;std::uint64_t total{};for(auto value:found->second)total+=value;return total/found->second.size();}

std::string UsageForecast::report() const {std::vector<std::vector<std::string>> rows{{"session","prediction"}};for(const auto&[session,_]:samples_)rows.push_back({std::to_string(session),std::to_string(predict(session))});return render_table(rows);}

void UsageForecast::clear(){samples_.clear();}

}  // namespace aegis::detail
