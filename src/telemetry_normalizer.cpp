#include "internal.hpp"

namespace aegis::detail {

std::map<std::string,std::uint64_t> TelemetryNormalizer::normalize(const std::map<std::string,std::uint64_t>& raw) const {std::map<std::string,std::uint64_t> out;for(const auto&[key,value]:raw){const auto canonical=canonical_text(key);if(canonical.empty())continue;out[canonical]+=value;}return out;}

std::string TelemetryNormalizer::render(const std::map<std::string,std::uint64_t>& values) const {std::vector<std::vector<std::string>> rows{{"metric","value"}};for(const auto&[key,value]:values)rows.push_back({key,std::to_string(value)});return render_table(rows);}

}  // namespace aegis::detail
