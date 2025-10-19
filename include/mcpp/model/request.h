#pragma once

#include "mcpp/core/json_rpc_message.h"
#include <string>
#include <optional>
#include <nlohmann/json.hpp>

namespace mcpp::model {

/**
 * @brief Base MCP request
 */
class McpRequest {
public:
    McpRequest() = default;
    virtual ~McpRequest() = default;

    /**
     * @brief Convert to JSON-RPC request
     */
    virtual std::unique_ptr<core::JsonRpcRequest> to_json_rpc_request() const = 0;

    /**
     * @brief Validate request parameters
     */
    virtual bool is_valid() const = 0;

    /**
     * @brief Get method name
     */
    virtual std::string get_method() const = 0;
};

/**
 * @brief Initialize request
 */
class InitializeRequest : public McpRequest {
public:
    struct Params {
        std::string protocol_version;
        nlohmann::json capabilities;
        std::optional<std::string> client_info;
        std::optional<nlohmann::json> trace;

        nlohmann::json to_json() const;
        static Params from_json(const nlohmann::json& j);
    };

    explicit InitializeRequest(const Params& params);

    std::unique_ptr<core::JsonRpcRequest> to_json_rpc_request() const override;
    bool is_valid() const override;
    std::string get_method() const override { return "initialize"; }

    const Params& get_params() const { return params_; }

private:
    Params params_;
};

/**
 * @brief Ping request
 */
class PingRequest : public McpRequest {
public:
    PingRequest() = default;

    std::unique_ptr<core::JsonRpcRequest> to_json_rpc_request() const override;
    bool is_valid() const override { return true; }
    std::string get_method() const override { return "ping"; }
};

/**
 * @brief Tools list request
 */
class ListToolsRequest : public McpRequest {
public:
    ListToolsRequest() = default;

    std::unique_ptr<core::JsonRpcRequest> to_json_rpc_request() const override;
    bool is_valid() const override { return true; }
    std::string get_method() const override { return "tools/list"; }
};

/**
 * @brief Tool call request
 */
class CallToolRequest : public McpRequest {
public:
    CallToolRequest(const std::string& name, const nlohmann::json& arguments = {});

    std::unique_ptr<core::JsonRpcRequest> to_json_rpc_request() const override;
    bool is_valid() const override;
    std::string get_method() const override { return "tools/call"; }

    const std::string& get_name() const { return name_; }
    const nlohmann::json& get_arguments() const { return arguments_; }

private:
    std::string name_;
    nlohmann::json arguments_;
};

} // namespace mcpp::model