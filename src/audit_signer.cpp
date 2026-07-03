#include "internal.hpp"

namespace aegis::detail {

std::string AuditSigner::sign(std::string_view value,std::string_view domain) const{return stable_digest(std::string(domain)+":"+std::string(value));}

bool AuditSigner::verify(std::string_view value,std::string_view domain,std::string_view signature) const{return sign(value,domain)==signature;}

}  // namespace aegis::detail
