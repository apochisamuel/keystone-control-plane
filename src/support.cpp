#include "internal.hpp"

namespace aegis::detail {

std::string describe_operation(Operation operation) {
    switch (operation) {
        case Operation::Open: return "open";
        case Operation::Register: return "register";
        case Operation::StagePolicy: return "stage";
        case Operation::Commit: return "commit";
        case Operation::Data: return "data";
        case Operation::Handover: return "handover";
        case Operation::Rollback: return "rollback";
        case Operation::Close: return "close";
        case Operation::Topology: return "topology";
        case Operation::Audit: return "audit";
        case Operation::Snapshot: return "snapshot";
        case Operation::Recover: return "recover";
        case Operation::Extensions: return "extensions";
        case Operation::Heartbeat: return "heartbeat";
    }
    return "unknown";
}

std::string stable_digest(std::string_view text) {
    const auto data = std::span<const Byte>(reinterpret_cast<const Byte*>(text.data()), text.size());
    const auto first = checksum32(data);
    const auto second = crc32(data);
    std::ostringstream output;
    output << std::hex << std::setw(8) << std::setfill('0') << first
           << std::setw(8) << std::setfill('0') << second
           << std::dec << std::setfill(' ');
    return output.str();
}

std::string render_table(const std::vector<std::vector<std::string>>& rows) {
    if (rows.empty()) return {};
    std::vector<std::size_t> widths;
    for (const auto& row : rows) {
        if (widths.size() < row.size()) widths.resize(row.size(), 0);
        for (std::size_t index = 0; index < row.size(); ++index) widths[index] = std::max(widths[index], row[index].size());
    }
    std::ostringstream output;
    for (std::size_t row_index = 0; row_index < rows.size(); ++row_index) {
        const auto& row = rows[row_index];
        for (std::size_t column = 0; column < widths.size(); ++column) {
            const auto value = column < row.size() ? row[column] : std::string{};
            if (column != 0) output << " | ";
            output << std::left << std::setw(static_cast<int>(widths[column])) << value;
        }
        output << '\n';
        if (row_index == 0) {
            for (std::size_t column = 0; column < widths.size(); ++column) {
                if (column != 0) output << "-+-";
                output << std::string(widths[column], '-');
            }
            output << '\n';
        }
    }
    return output.str();
}

}  // namespace aegis::detail
