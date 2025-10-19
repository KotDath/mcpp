#pragma once

#include <string>
#include <variant>
#include <optional>
#include <nlohmann/json.hpp>

namespace mcpp::core {

/**
 * @brief JSON-RPC 2.0 message types
 */
enum class JsonRpcMessageType {
    Request,
    Response,
    Notification
};

/**
 * @brief JSON-RPC 2.0 error codes
 */
enum class JsonRpcErrorCode {
    ParseError = -32700,
    InvalidRequest = -32600,
    MethodNotFound = -32601,
    InvalidParams = -32602,
    InternalError = -32603,
    ServerErrorStart = -32000,
    ServerErrorEnd = -32099
};

/**
 * @brief JSON-RPC 2.0 error object
 */
struct JsonRpcError {
    JsonRpcErrorCode code;
    std::string message;
    std::optional<nlohmann::json> data;

    nlohmann::json to_json() const;
    static JsonRpcError from_json(const nlohmann::json& j);
};

/**
 * @brief Base JSON-RPC 2.0 message
 */
class JsonRpcMessage {
public:
    static constexpr const char* JSON_RPC_VERSION = "2.0";

    JsonRpcMessage() = default;
    virtual ~JsonRpcMessage() = default;

    JsonRpcMessage(const JsonRpcMessage&) = default;
    JsonRpcMessage& operator=(const JsonRpcMessage&) = default;
    JsonRpcMessage(JsonRpcMessage&&) = default;
    JsonRpcMessage& operator=(JsonRpcMessage&&) = default;

    /**
     * @brief Get message type
     */
    virtual JsonRpcMessageType get_type() const = 0;

    /**
     * @brief Convert to JSON
     */
    virtual nlohmann::json to_json() const = 0;

    /**
     * @brief Validate message structure
     */
    virtual bool is_valid() const = 0;

    /**
     * @brief Parse from JSON
     */
    static std::unique_ptr<JsonRpcMessage> from_json(const nlohmann::json& j);

protected:
    /**
     * @brief Validate common JSON-RPC fields
     */
    bool validate_base(const nlohmann::json& j) const;
};

/**
 * @brief JSON-RPC 2.0 request message
 */
class JsonRpcRequest : public JsonRpcMessage {
public:
    using IdType = std::variant<std::nullptr_t, std::string, int64_t>;

    JsonRpcRequest(const IdType& id, const std::string& method,
                   const nlohmann::json& params = nlohmann::json{});

    JsonRpcMessageType get_type() const override { return JsonRpcMessageType::Request; }
    nlohmann::json to_json() const override;
    bool is_valid() const override;

    const IdType& get_id() const { return id_; }
    const std::string& get_method() const { return method_; }
    const nlohmann::json& get_params() const { return params_; }

    void set_method(const std::string& method) { method_ = method; }
    void set_params(const nlohmann::json& params) { params_ = params; }

private:
    IdType id_;
    std::string method_;
    nlohmann::json params_;
};

/**
 * @brief JSON-RPC 2.0 response message
 */
class JsonRpcResponse : public JsonRpcMessage {
public:
    using IdType = std::variant<std::string, int64_t>;

    JsonRpcResponse(const IdType& id, const nlohmann::json& result);
    JsonRpcResponse(const IdType& id, const JsonRpcError& error);

    JsonRpcMessageType get_type() const override { return JsonRpcMessageType::Response; }
    nlohmann::json to_json() const override;
    bool is_valid() const override;

    const IdType& get_id() const { return id_; }
    const nlohmann::json& get_result() const { return result_; }
    const std::optional<JsonRpcError>& get_error() const { return error_; }

    bool is_error() const { return error_.has_value(); }

private:
    IdType id_;
    nlohmann::json result_;
    std::optional<JsonRpcError> error_;
};

/**
 * @brief JSON-RPC 2.0 notification message
 */
class JsonRpcNotification : public JsonRpcMessage {
public:
    JsonRpcNotification(const std::string& method,
                       const nlohmann::json& params = nlohmann::json{});

    JsonRpcMessageType get_type() const override { return JsonRpcMessageType::Notification; }
    nlohmann::json to_json() const override;
    bool is_valid() const override;

    const std::string& get_method() const { return method_; }
    const nlohmann::json& get_params() const { return params_; }

    void set_method(const std::string& method) { method_ = method; }
    void set_params(const nlohmann::json& params) { params_ = params; }

private:
    std::string method_;
    nlohmann::json params_;
};

// Helper functions for ID conversion
JsonRpcRequest::IdType json_to_id(const nlohmann::json& j);
nlohmann::json id_to_json(const JsonRpcRequest::IdType& id);
nlohmann::json response_id_to_json(const JsonRpcResponse::IdType& id);

} // namespace mcpp::core