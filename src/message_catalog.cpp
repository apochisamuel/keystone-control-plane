#include "internal.hpp"

namespace aegis::detail {

void MessageCatalog::record(Operation operation) { ++counts_[operation]; }

std::uint64_t MessageCatalog::count(Operation operation) const { const auto found=counts_.find(operation); return found==counts_.end()?0U:found->second; }

std::string MessageCatalog::summary() const { std::vector<std::vector<std::string>> rows{{"operation","count"}}; for (const auto& [op,value] : counts_) rows.push_back({describe_operation(op),std::to_string(value)}); return render_table(rows); }

void MessageCatalog::clear() { counts_.clear(); }

}  // namespace aegis::detail
