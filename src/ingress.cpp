#include "internal.hpp"

namespace aegis::detail {

bool IngressFilter::allow(const Frame& frame, std::size_t raw_size, std::string& reason) {
    auto& source = sources_[frame.session_id];
    if (frame.session_id == 0U && frame.operation != Operation::Heartbeat) {
        reason = "zero session source";
        source.rejected++;
        ++rejected_;
        return false;
    }
    if (raw_size > 1024U * 1024U) {
        reason = "frame exceeds ingress budget";
        source.rejected++;
        ++rejected_;
        return false;
    }
    if (source.bytes > 4U * 1024U * 1024U - raw_size) {
        reason = "source byte budget exhausted";
        source.rejected++;
        ++rejected_;
        return false;
    }
    if (source.accepted + source.rejected > 100000U) {
        reason = "source event budget exhausted";
        source.rejected++;
        ++rejected_;
        return false;
    }
    source.bytes += raw_size;
    reason.clear();
    return true;
}

void IngressFilter::observe_result(const Frame& frame, bool accepted) {
    auto& source = sources_[frame.session_id];
    if (accepted) source.accepted++;
    else { source.rejected++; ++rejected_; }
}

std::size_t IngressFilter::rejected() const { return rejected_; }

std::string IngressFilter::report() const {
    std::vector<std::uint32_t> ids;
    for (const auto& [id, state] : sources_) { (void)state; ids.push_back(id); }
    std::sort(ids.begin(), ids.end());
    std::ostringstream output;
    output << "ingress.sources=" << ids.size() << ";rejected=" << rejected_ << ";";
    for (const auto id : ids) {
        const auto& state = sources_.at(id);
        output << id << ":bytes=" << state.bytes << ":ok=" << state.accepted << ":bad=" << state.rejected << ";";
    }
    return output.str();
}

void IngressFilter::clear() { sources_.clear(); rejected_ = 0; }

}  // namespace aegis::detail
