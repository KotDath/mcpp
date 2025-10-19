#include "mcpp/core/protocol_version.h"
#include <algorithm>

namespace mcpp::core {

ProtocolVersion::ProtocolVersion(const std::string& version)
    : version_(version) {
}

bool ProtocolVersion::is_supported() const {
    const auto& supported = get_supported_versions();
    return std::find(supported.begin(), supported.end(), version_) != supported.end();
}

std::vector<std::string> ProtocolVersion::get_supported_versions() {
    return {V_2024_11_05, V_2025_03_26, V_2025_06_18};
}

bool ProtocolVersion::operator==(const ProtocolVersion& other) const {
    return version_rank(version_) == version_rank(other.version_);
}

bool ProtocolVersion::operator!=(const ProtocolVersion& other) const {
    return !(*this == other);
}

bool ProtocolVersion::operator<(const ProtocolVersion& other) const {
    return version_rank(version_) < version_rank(other.version_);
}

bool ProtocolVersion::operator<=(const ProtocolVersion& other) const {
    return version_rank(version_) <= version_rank(other.version_);
}

bool ProtocolVersion::operator>(const ProtocolVersion& other) const {
    return version_rank(version_) > version_rank(other.version_);
}

bool ProtocolVersion::operator>=(const ProtocolVersion& other) const {
    return version_rank(version_) >= version_rank(other.version_);
}

int ProtocolVersion::version_rank(const std::string& version) {
    if (version == V_2024_11_05) return 1;
    if (version == V_2025_03_26) return 2;
    if (version == V_2025_06_18) return 3;
    return 0; // Unknown version
}

} // namespace mcpp::core