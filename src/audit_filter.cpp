#include "internal.hpp"

namespace aegis::detail {

void AuditFilter::allow(std::string component){if(!component.empty())allowed_.insert(std::move(component));}

bool AuditFilter::accepts(std::string_view component) const{return allowed_.empty()||allowed_.contains(std::string(component));}

void AuditFilter::record(std::string component){if(accepts(component))++observed_[std::move(component)];}

std::string AuditFilter::summary() const {std::vector<std::vector<std::string>> rows{{"component","count"}};for(const auto&[component,count]:observed_)rows.push_back({component,std::to_string(count)});return render_table(rows);}

void AuditFilter::clear(){allowed_.clear();observed_.clear();}

}  // namespace aegis::detail
