#pragma once

#include "mcpp/core/json_rpc_message.h"
#include <string>
#include <optional>
#include <nlohmann/json.hpp>

namespace mcpp::model {

/**
 * @brief Base MCP response
 */
class McpResponse {
public:
    McpResponse() = default;
    virtual ~McpResponse() = default;

    /**
     * @brief Convert to JSON-RPC response
     */
    virtual std::unique_ptr<core::JsonRpcResponse> to_json_rpc_response() const = 0;

    /**
     * @brief Validate response data
     */
    virtual bool is_valid() const = 0;

    /**
     * @brief Get response ID
     */
    virtual core::JsonRpcResponse::IdType get_id() const = 0;
};

/**
 * @brief Initialize response
 */
class InitializeResponse : public McpResponse {
public:
    struct Result {
        std::string protocol_version;
        nlohmann::json capabilities;
        std::optional<std::string> server_info;
        std::optional<nlohmann::json> trace;

        nlohmann::json to_json() const;
        static Result from_json(const nlohmann::json& j);
    };

    explicit InitializeResponse(const Result& result,
                                const core::JsonRpcResponse::IdType& id = 1);

    std::unique_ptr<core::JsonRpcResponse> to_json_rpc_response() const override;
    bool is_valid() const override;
    core::JsonRpcResponse::IdType get_id() const override { return id_; }

    const Result& get_result() const { return result_; }

private:
    Result result_;
    core::JsonRpcResponse::IdType id_;
};

/**
 * @brief Ping response
 */
class PingResponse : public McpResponse {
public:
    explicit PingResponse(const core::JsonRpcResponse::IdType& id = 2);

    std::unique_ptr<core::JsonRpcResponse> to_json_rpc_response() const override;
    bool is_valid() const override { return true; }
    core::JsonRpcResponse::IdType get_id() const override { return id_; }

private:
    core::JsonRpcResponse::IdType id_;
};

/**
 * @brief Tools list response
 */
class ListToolsResponse : public McpResponse {
public:
    explicit ListToolsResponse(const nlohmann::json& tools,
                               const core::JsonRpcResponse::IdType& id = 3);

    std::unique_ptr<core::JsonRpcResponse> to_json_rpc_response() const override;
    bool is_valid() const override;
    core::JsonRpcResponse::IdType get_id() const override { return id_; }

    const nlohmann::json& get_tools() const { return tools_; }

private:
    nlohmann::json tools_;
    core::JsonRpcResponse::IdType id_;
};

/**
 * @brief Tool call response
 */
class CallToolResponse : public McpResponse {
public:
    explicit CallToolResponse(const nlohmann::json& result,
                              const core::JsonRpcResponse::IdType& id = 4);

    std::unique_ptr<core::JsonRpcResponse> to_json_rpc_response() const override;
    bool is_valid() const override;
    core::JsonRpcResponse::IdType get_id() const override { return id_; }

    const nlohmann::json& get_result() const { return result_; }

private:
    nlohmann::json result_;
    core::JsonRpcResponse::IdType id_;
};

} // namespace mcpp::model