#include "internal.hpp"

namespace aegis::detail {

CongestionAdjustment CongestionControlLoop::adjust(std::string path,std::uint64_t offered,std::uint64_t delivered,std::uint64_t allowance){const auto deficit=offered>delivered?offered-delivered:0U;CongestionAdjustment result;result.path=path;if(offered==0U||deficit*4U<offered){result.allowance=allowance+std::max<std::uint64_t>(1U,allowance/16U);result.action="increase";}else if(deficit*2U>=offered){result.allowance=std::max<std::uint64_t>(1U,allowance/2U);result.action="decrease";}else{result.allowance=allowance;result.action="hold";}adjustments_[std::move(path)]=result;return result;}

std::optional<CongestionAdjustment> CongestionControlLoop::latest(std::string_view path) const {const auto found=adjustments_.find(std::string(path));return found==adjustments_.end()?std::nullopt:std::optional<CongestionAdjustment>(found->second);}

void CongestionControlLoop::clear(){adjustments_.clear();}

}  // namespace aegis::detail
