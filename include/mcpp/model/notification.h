#pragma once

#include "mcpp/core/json_rpc_message.h"
#include <string>
#include <optional>
#include <nlohmann/json.hpp>

namespace mcpp::model {

/**
 * @brief Base MCP notification
 */
class McpNotification {
public:
    McpNotification() = default;
    virtual ~McpNotification() = default;

    /**
     * @brief Convert to JSON-RPC notification
     */
    virtual std::unique_ptr<core::JsonRpcNotification> to_json_rpc_notification() const = 0;

    /**
     * @brief Validate notification data
     */
    virtual bool is_valid() const = 0;

    /**
     * @brief Get notification method
     */
    virtual std::string get_method() const = 0;
};

/**
 * @brief Initialized notification
 */
class InitializedNotification : public McpNotification {
public:
    InitializedNotification() = default;

    std::unique_ptr<core::JsonRpcNotification> to_json_rpc_notification() const override;
    bool is_valid() const override { return true; }
    std::string get_method() const override { return "notifications/initialized"; }
};

/**
 * @brief Cancelled notification
 */
class CancelledNotification : public McpNotification {
public:
    explicit CancelledNotification(core::JsonRpcRequest::IdType request_id);

    std::unique_ptr<core::JsonRpcNotification> to_json_rpc_notification() const override;
    bool is_valid() const override;
    std::string get_method() const override { return "notifications/cancelled"; }

    const core::JsonRpcRequest::IdType& get_request_id() const { return request_id_; }

private:
    core::JsonRpcRequest::IdType request_id_;
};

/**
 * @brief Progress notification
 */
class ProgressNotification : public McpNotification {
public:
    struct Params {
        core::JsonRpcRequest::IdType request_id;
        nlohmann::json progress;
        std::optional<double> total;

        nlohmann::json to_json() const;
        static Params from_json(const nlohmann::json& j);
    };

    explicit ProgressNotification(const Params& params);

    std::unique_ptr<core::JsonRpcNotification> to_json_rpc_notification() const override;
    bool is_valid() const override;
    std::string get_method() const override { return "notifications/progress"; }

    const Params& get_params() const { return params_; }

private:
    Params params_;
};

/**
 * @brief Message notification
 */
class MessageNotification : public McpNotification {
public:
    enum class Level {
        Debug,
        Info,
        Warning,
        Error
    };

    struct Params {
        Level level;
        std::string message;
        std::optional<std::string> logger;

        nlohmann::json to_json() const;
        static Params from_json(const nlohmann::json& j);

    private:
        static std::string level_to_string(Level level);
        static Level string_to_level(const std::string& str);
    };

    explicit MessageNotification(const Params& params);

    std::unique_ptr<core::JsonRpcNotification> to_json_rpc_notification() const override;
    bool is_valid() const override;
    std::string get_method() const override { return "notifications/message"; }

    const Params& get_params() const { return params_; }

private:
    Params params_;
};

} // namespace mcpp::model