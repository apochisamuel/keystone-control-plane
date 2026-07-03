#include "internal.hpp"

namespace aegis::detail {

void CaptureAnalyzer::observe(std::uint64_t ordinal, const Frame& frame, std::size_t bytes, std::uint32_t checksum) {
    CaptureObservation observation;
    observation.ordinal = ordinal;
    observation.session_id = frame.session_id;
    observation.operation = frame.operation;
    observation.fields = frame.fields.size();
    observation.bytes = bytes;
    observation.checksum = checksum;
    observations_.push_back(std::move(observation));
    if (observations_.size() > 4096U) observations_.erase(observations_.begin(), observations_.begin() + 512);
}

std::vector<CaptureObservation> CaptureAnalyzer::session(std::uint32_t session_id) const {
    std::vector<CaptureObservation> result;
    for (const auto& observation : observations_) if (observation.session_id == session_id) result.push_back(observation);
    return result;
}

std::unordered_map<std::string, std::size_t> CaptureAnalyzer::operation_counts() const {
    std::unordered_map<std::string, std::size_t> result;
    for (const auto& observation : observations_) result[describe_operation(observation.operation)]++;
    return result;
}

std::string CaptureAnalyzer::report() const {
    auto counts = operation_counts();
    std::vector<std::string> operations;
    for (const auto& [operation, count] : counts) { (void)count; operations.push_back(operation); }
    std::sort(operations.begin(), operations.end());
    std::ostringstream output;
    output << "capture.observations=" << observations_.size() << ";";
    for (const auto& operation : operations) output << operation << "=" << counts[operation] << ";";
    return output.str();
}

void CaptureAnalyzer::clear() { observations_.clear(); }

}  // namespace aegis::detail
