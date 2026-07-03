#include "internal.hpp"

namespace aegis::detail {

std::vector<CaptureExportRow> CaptureExporter::rows(const std::vector<CaptureObservation>& observations,
                                                     const SessionStore& sessions) const {
    std::vector<CaptureExportRow> result;
    for (const auto& observation : observations) {
        CaptureExportRow row;
        row.ordinal = observation.ordinal;
        row.session_id = observation.session_id;
        row.operation = describe_operation(observation.operation);
        if (const auto* session = sessions.find(observation.session_id); session != nullptr) row.generation = session->generation;
        row.summary = "fields=" + std::to_string(observation.fields) + ";bytes=" + std::to_string(observation.bytes) + ";checksum=" + std::to_string(observation.checksum);
        result.push_back(std::move(row));
    }
    return result;
}

std::string CaptureExporter::csv(const std::vector<CaptureExportRow>& rows) const {
    std::ostringstream output;
    output << "ordinal,session,operation,generation,summary\n";
    for (const auto& row : rows) {
        auto escaped = row.summary;
        std::replace(escaped.begin(), escaped.end(), '"', '_');
        output << row.ordinal << "," << row.session_id << "," << row.operation << "," << row.generation << ",\"" << escaped << "\"\n";
    }
    return output.str();
}

}  // namespace aegis::detail
