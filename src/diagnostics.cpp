#include "internal.hpp"

namespace aegis::detail {

namespace {
class RenderBuffer {
public:
    explicit RenderBuffer(std::size_t capacity) : bytes_(new char[capacity == 0 ? 1U : capacity]), capacity_(capacity == 0 ? 1U : capacity) {}
    void append(char value) { bytes_[used_++] = value; }
    void append(std::string_view value) { for (const auto ch : value) append(ch); }
    std::string take() const { return {bytes_.get(), used_}; }
private:
    std::unique_ptr<char[]> bytes_;
    std::size_t capacity_{};
    std::size_t used_{};
};

void append_escaped(RenderBuffer& buffer, std::string_view value) {
    for (const unsigned char byte : value) {
        if (byte == '"' || byte == '\\') {
            buffer.append('\\');
            buffer.append(static_cast<char>(byte));
        } else if (byte < 0x20U) {
            static constexpr char digits[] = "0123456789abcdef";
            buffer.append('\\'); buffer.append('u'); buffer.append('0'); buffer.append('0');
            buffer.append(digits[(byte >> 4U) & 0x0fU]);
            buffer.append(digits[byte & 0x0fU]);
        } else {
            buffer.append(static_cast<char>(byte));
        }
    }
}
}

std::string Diagnostics::render_session(const Session& session, const CompiledPolicy* policy) const {
    const auto profile_name = session.profile == nullptr ? std::string("none") : session.profile->owner;
    const auto hop = (session.profile == nullptr || session.profile->routes.empty()) ? std::string("none") : session.profile->routes.front().next_hop;
    const auto extension = policy == nullptr ? std::string("none") : extension_summary(policy->extensions);
    const std::size_t planned = profile_name.size() + hop.size() + extension.size() + 64U;
    RenderBuffer buffer(planned);
    buffer.append('{');
    buffer.append("\"session\":"); buffer.append(std::to_string(session.id));
    buffer.append(",\"profile\":\""); append_escaped(buffer, profile_name); buffer.append('"');
    buffer.append(",\"hop\":\""); append_escaped(buffer, hop); buffer.append('"');
    buffer.append(",\"extension\":\""); append_escaped(buffer, extension); buffer.append('"');
    buffer.append(",\"generation\":"); buffer.append(std::to_string(session.generation));
    buffer.append('}');
    return buffer.take();
}

std::string Diagnostics::render_engine(const std::vector<const Session*>& sessions, const AuditJournal& audit) const {
    std::ostringstream output;
    output << "sessions=" << sessions.size() << "\n";
    for (const auto* session : sessions) {
        if (session == nullptr) continue;
        output << session->id << ':' << (session->profile == nullptr ? "none" : session->profile->owner)
               << ':' << session->generation << '\n';
    }
    output << audit.render();
    return output.str();
}

std::vector<std::string> Diagnostics::validate_session(const Session& session) const {
    std::vector<std::string> issues;
    if (session.id == 0U) issues.push_back("session id is zero");
    if (!session.closed && session.profile == nullptr) issues.push_back("open session lacks profile");
    if (session.profile != nullptr && session.profile->routes.empty()) issues.push_back("profile lacks route");
    if (session.quota == 0U && !session.closed) issues.push_back("quota exhausted");
    return issues;
}

}  // namespace aegis::detail
