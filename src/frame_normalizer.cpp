#include "internal.hpp"

namespace aegis::detail {

Frame FrameNormalizer::normalize(const Frame& frame) const {
    Frame result;
    result.operation = frame.operation;
    result.flags = frame.flags;
    result.session_id = frame.session_id;
    std::unordered_map<Byte, Field> by_tag;
    for (const auto& field : frame.fields) {
        auto copy = field;
        if (field.tag == static_cast<Byte>(FieldTag::NextHop) || field.tag == static_cast<Byte>(FieldTag::Label) || field.tag == static_cast<Byte>(FieldTag::Subscriber)) {
            const auto text = canonical_text(std::string_view(reinterpret_cast<const char*>(field.value.data()), field.value.size()));
            copy.value.assign(text.begin(), text.end());
        }
        by_tag[field.tag] = std::move(copy);
    }
    std::vector<Byte> tags;
    tags.reserve(by_tag.size());
    for (const auto& [tag, field] : by_tag) { (void)field; tags.push_back(tag); }
    std::sort(tags.begin(), tags.end());
    for (const auto tag : tags) result.fields.push_back(std::move(by_tag.at(tag)));
    return result;
}

std::vector<std::string> FrameNormalizer::validate(const Frame& frame) const {
    std::vector<std::string> errors;
    if (frame.operation == Operation::Open && frame.session_id == 0U) errors.push_back("open zero session");
    if (frame.fields.size() > 128U) errors.push_back("too many fields");
    std::set<Byte> tags;
    for (const auto& field : frame.fields) {
        if (!tags.insert(field.tag).second) errors.push_back("duplicate field:" + std::to_string(field.tag));
        if (field.value.size() > 65535U) errors.push_back("field exceeds encoding");
    }
    return errors;
}

std::string FrameNormalizer::describe(const Frame& frame) const {
    std::ostringstream output;
    output << describe_operation(frame.operation) << ";session=" << frame.session_id << ";flags=" << static_cast<unsigned>(frame.flags) << ";fields=" << frame.fields.size();
    return output.str();
}

}  // namespace aegis::detail
