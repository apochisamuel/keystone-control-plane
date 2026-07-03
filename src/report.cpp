#include "internal.hpp"

namespace aegis::detail {

void ReportAssembler::section(std::string name) {
    name = canonical_text(name);
    if (name.empty()) name = "general";
    sections_.push_back(ReportSection{std::move(name), {}});
}

void ReportAssembler::field(std::string key, std::string value) {
    if (sections_.empty()) section("general");
    key = canonical_text(key);
    if (key.empty()) return;
    sections_.back().rows.emplace_back(std::move(key), std::move(value));
}

void ReportAssembler::field(std::string key, std::uint64_t value) {
    field(std::move(key), std::to_string(value));
}

std::string ReportAssembler::text() const {
    std::ostringstream output;
    for (const auto& section : sections_) {
        output << "[" << section.name << "]\n";
        for (const auto& [key, value] : section.rows) output << key << "=" << value << "\n";
    }
    return output.str();
}

std::string ReportAssembler::json() const {
    std::ostringstream output;
    output << "{";
    for (std::size_t section_index = 0; section_index < sections_.size(); ++section_index) {
        if (section_index != 0U) output << ",";
        const auto& section = sections_[section_index];
        output << "\"" << escaped_json(section.name) << "\":{";
        for (std::size_t row_index = 0; row_index < section.rows.size(); ++row_index) {
            if (row_index != 0U) output << ",";
            output << "\"" << escaped_json(section.rows[row_index].first) << "\":\""
                   << escaped_json(section.rows[row_index].second) << "\"";
        }
        output << "}";
    }
    output << "}";
    return output.str();
}

void ReportAssembler::clear() { sections_.clear(); }

}  // namespace aegis::detail
