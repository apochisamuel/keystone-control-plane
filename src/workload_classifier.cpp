#include "internal.hpp"

namespace aegis::detail {

std::string WorkloadClassifier::classify(std::span<const Byte> payload) const {if(payload.empty())return "empty";std::size_t printable{};std::size_t zeroes{};for(auto byte:payload){if(byte==0U)++zeroes;if(byte>=32U&&byte<127U)++printable;}if(zeroes*2U>payload.size())return "sparse";if(printable*100U/payload.size()>=80U)return "text";if(payload.size()>1024U)return "bulk";return "binary";}

void WorkloadClassifier::observe(std::string category){++counts_[std::move(category)];}

std::map<std::string,std::uint64_t> WorkloadClassifier::histogram() const{return counts_;}

void WorkloadClassifier::clear(){counts_.clear();}

}  // namespace aegis::detail
