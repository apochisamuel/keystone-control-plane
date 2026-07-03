#include "internal.hpp"

namespace aegis::detail {

std::vector<std::string> ConfigurationCatalog::validate(const std::unordered_map<std::string, std::string>& values) const {
    std::vector<std::string> errors;
    for (const auto& [key, value] : values) {
        const auto normalized = canonical_text(key);
        if (normalized.empty()) errors.push_back("empty configuration key");
        if (value.size() > 4096U) errors.push_back("value too large:" + normalized);
        if (normalized.find("secret") != std::string::npos || normalized.find("password") != std::string::npos) {
            errors.push_back("unsupported sensitive configuration:" + normalized);
        }
        if (normalized == "quota.default") {
            try {
                if (std::stoull(value) == 0U) errors.push_back("quota.default must be positive");
            } catch (const std::exception&) { errors.push_back("quota.default must be numeric"); }
        }
        if (normalized == "scheduler.max-priority") {
            try {
                if (std::stoull(value) > 100U) errors.push_back("scheduler.max-priority exceeds 100");
            } catch (const std::exception&) { errors.push_back("scheduler.max-priority must be numeric"); }
        }
    }
    std::sort(errors.begin(), errors.end());
    return errors;
}

std::uint64_t ConfigurationCatalog::publish(std::unordered_map<std::string, std::string> values) {
    const auto errors = validate(values);
    if (!errors.empty()) return 0U;
    ConfigRevision revision;
    revision.version = next_version_++;
    for (auto& [key, value] : values) revision.values.emplace(canonical_text(key), value);
    std::vector<std::string> canonical;
    canonical.reserve(revision.values.size());
    for (const auto& [key, value] : revision.values) canonical.push_back(key + "=" + value);
    std::sort(canonical.begin(), canonical.end());
    revision.digest = stable_digest(join_labels(canonical, "|"));
    revisions_.emplace(revision.version, revision);
    return revision.version;
}

std::optional<std::string> ConfigurationCatalog::get(std::uint64_t version, std::string_view key) const {
    const auto revision_it = revisions_.find(version == 0U ? current() : version);
    if (revision_it == revisions_.end()) return std::nullopt;
    const auto value_it = revision_it->second.values.find(canonical_text(key));
    return value_it == revision_it->second.values.end() ? std::nullopt : std::optional<std::string>(value_it->second);
}

std::optional<ConfigRevision> ConfigurationCatalog::revision(std::uint64_t version) const {
    const auto iterator = revisions_.find(version);
    return iterator == revisions_.end() ? std::nullopt : std::optional<ConfigRevision>(iterator->second);
}

std::uint64_t ConfigurationCatalog::current() const {
    if (revisions_.empty()) return 0U;
    std::uint64_t result = 0;
    for (const auto& [version, revision] : revisions_) { (void)revision; result = std::max(result, version); }
    return result;
}

std::string ConfigurationCatalog::report() const {
    std::ostringstream output;
    output << "config.revisions=" << revisions_.size() << ";current=" << current() << ";";
    if (const auto current_revision = revision(current()); current_revision.has_value()) output << "digest=" << current_revision->digest << ";";
    return output.str();
}

void ConfigurationCatalog::clear() { next_version_ = 1; revisions_.clear(); }

}  // namespace aegis::detail
