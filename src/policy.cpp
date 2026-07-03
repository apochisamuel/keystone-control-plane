#include "internal.hpp"

namespace aegis::detail {

std::vector<RouteRule> PolicyCompiler::build_rules(const PolicyCandidate& candidate) const {
    std::vector<RouteRule> rules;
    const auto labels = split_labels(candidate.label);
    if (labels.empty()) {
        rules.push_back(RouteRule{candidate.route_id, 0, "default", candidate.version});
    } else {
        for (std::size_t index = 0; index < labels.size(); ++index) {
            RouteRule rule;
            rule.route_id = candidate.route_id;
            rule.quota = static_cast<std::uint32_t>((index + 1U) * 100U);
            rule.match = labels[index];
            rule.version = candidate.version;
            rules.push_back(std::move(rule));
        }
    }
    return rules;
}

std::string PolicyCompiler::digest(const PolicyCandidate& candidate) const {
    std::ostringstream text;
    text << candidate.session_id << ':' << candidate.profile_id << ':' << candidate.route_id << ':'
         << candidate.version << ':' << canonical_text(candidate.next_hop) << ':' << canonical_text(candidate.label)
         << ':' << hex(candidate.extension) << ':' << candidate.route_mode;
    return stable_digest(text.str());
}

CompiledPolicy PolicyCompiler::compile(const PolicyCandidate& candidate) const {
    const auto issues = validate_policy_candidate(candidate);
    if (!issues.empty()) throw DecodeError("policy candidate rejected: " + issues.front());
    CompiledPolicy compiled;
    compiled.session_id = candidate.session_id;
    compiled.version = candidate.version;
    compiled.route_id = candidate.route_id;
    compiled.route_label = canonical_text(candidate.label);
    compiled.route_mode = candidate.route_mode;
    compiled.extensions = decode_extensions(candidate.extension);
    canonicalize_extensions(compiled.extensions, compiled.route_mode);
    compiled.rules = build_rules(candidate);
    compiled.attachment = std::make_unique<PolicyAttachment>();
    compiled.attachment->name = "policy-" + std::to_string(candidate.session_id);
    compiled.attachment->bytes = candidate.extension;
    compiled.attachment->checksum = checksum32(candidate.extension);
    compiled.attachment->accounting[0] = candidate.version;
    compiled.attachment->accounting[1] = candidate.route_id;
    compiled.attachment->accounting[2] = compiled.rules.size();
    compiled.attachment->accounting[3] = crc32(candidate.extension);
    return compiled;
}

void AuditJournal::record(std::uint32_t session_id, std::string action, std::string digest) {
    AuditRecord record;
    record.session_id = session_id;
    record.action = canonical_text(action);
    record.sequence = next_sequence_++;
    record.digest = std::move(digest);
    records_.push_back(std::move(record));
}

void AuditJournal::record_attachment(std::uint32_t session_id, PolicyAttachment* attachment) {
    AuditRecord record;
    record.session_id = session_id;
    record.action = "policy attachment";
    record.sequence = next_sequence_++;
    record.digest = attachment == nullptr ? "none" : stable_digest(attachment->name);
    if (attachment != nullptr) record.attachment = std::unique_ptr<PolicyAttachment>(attachment);
    records_.push_back(std::move(record));
}

void AuditJournal::erase_session(std::uint32_t session_id) {
    records_.erase(std::remove_if(records_.begin(), records_.end(), [session_id](const AuditRecord& record) {
        return record.session_id == session_id;
    }), records_.end());
}

std::size_t AuditJournal::size() const {
    return records_.size();
}

std::vector<AuditRecord> AuditJournal::summaries() const {
    std::vector<AuditRecord> result;
    result.reserve(records_.size());
    for (const auto& record : records_) {
        AuditRecord copy;
        copy.session_id = record.session_id;
        copy.action = record.action;
        copy.sequence = record.sequence;
        copy.digest = record.digest;
        result.push_back(std::move(copy));
    }
    return result;
}

std::string AuditJournal::render() const {
    std::vector<std::vector<std::string>> rows;
    rows.push_back({"sequence", "session", "action", "digest"});
    for (const auto& record : records_) {
        rows.push_back({std::to_string(record.sequence), std::to_string(record.session_id), record.action, record.digest});
    }
    return render_table(rows);
}

void PolicyRepository::stage(PolicyCandidate candidate) {
    staged_[candidate.session_id] = std::move(candidate);
}

bool PolicyRepository::has_stage(std::uint32_t session_id) const {
    return staged_.contains(session_id);
}

std::optional<PolicyCandidate> PolicyRepository::staged(std::uint32_t session_id) const {
    const auto iterator = staged_.find(session_id);
    if (iterator == staged_.end()) return std::nullopt;
    return iterator->second;
}

void PolicyRepository::commit(std::uint32_t session_id) {
    const auto iterator = staged_.find(session_id);
    if (iterator == staged_.end()) throw DecodeError("no staged policy");
    auto compiled = std::make_unique<CompiledPolicy>(compiler_.compile(iterator->second));
    const bool mirror = iterator->second.mirror_audit;
    const auto digest = compiler_.digest(iterator->second);
    if (mirror) journal_.record_attachment(session_id, compiled->attachment.get());
    journal_.record(session_id, "policy commit", digest);
    active_[session_id] = std::move(compiled);
    staged_.erase(iterator);
}

void PolicyRepository::rollback(std::uint32_t session_id) {
    staged_.erase(session_id);
    journal_.erase_session(session_id);
    active_.erase(session_id);
}

const CompiledPolicy* PolicyRepository::active(std::uint32_t session_id) const {
    const auto iterator = active_.find(session_id);
    return iterator == active_.end() ? nullptr : iterator->second.get();
}

std::size_t PolicyRepository::active_count() const {
    return active_.size();
}

std::size_t PolicyRepository::staged_count() const {
    return staged_.size();
}

AuditJournal& PolicyRepository::journal() { return journal_; }
const AuditJournal& PolicyRepository::journal() const { return journal_; }

void PolicyRepository::clear() {
    staged_.clear();
    active_.clear();
    journal_ = AuditJournal{};
}

}  // namespace aegis::detail
