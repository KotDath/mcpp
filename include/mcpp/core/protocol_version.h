#pragma once

#include <string>
#include <vector>

namespace mcpp::core {

/**
 * @brief MCP Protocol Version
 *
 * Supports multiple versions of the Model Context Protocol.
 * Latest version is 2025-06-18.
 */
class ProtocolVersion {
public:
    static constexpr const char* V_2024_11_05 = "2024-11-05";
    static constexpr const char* V_2025_03_26 = "2025-03-26";
    static constexpr const char* V_2025_06_18 = "2025-06-18";
    static constexpr const char* LATEST = V_2025_06_18;

    /**
     * @brief Construct ProtocolVersion
     */
    explicit ProtocolVersion(const std::string& version = LATEST);

    /**
     * @brief Get version string
     */
    const std::string& to_string() const { return version_; }

    /**
     * @brief Check if this is the latest version
     */
    bool is_latest() const { return version_ == LATEST; }

    /**
     * @brief Check if version is supported
     */
    bool is_supported() const;

    /**
     * @brief Get all supported versions
     */
    static std::vector<std::string> get_supported_versions();

    /**
     * @brief Compare versions
     */
    bool operator==(const ProtocolVersion& other) const;
    bool operator!=(const ProtocolVersion& other) const;
    bool operator<(const ProtocolVersion& other) const;
    bool operator<=(const ProtocolVersion& other) const;
    bool operator>(const ProtocolVersion& other) const;
    bool operator>=(const ProtocolVersion& other) const;

private:
    std::string version_;

    static int version_rank(const std::string& version);
};

} // namespace mcpp::core