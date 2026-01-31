// mcpp - MCP C++ library
// https://github.com/mcpp-project/mcpp
//
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License

#include "mcpp/server/request_context.h"

#include <algorithm>
#include <nlohmann/json.hpp>

namespace mcpp {
namespace server {

RequestContext::RequestContext(
    const std::string& request_id,
    transport::Transport& transport
) : request_id_(request_id),
    transport_(transport),
    progress_token_(std::nullopt) {}

void RequestContext::set_progress_token(const std::string& token) {
    progress_token_ = token;
}

bool RequestContext::has_progress_token() const {
    return progress_token_.has_value();
}

const std::optional<std::string>& RequestContext::progress_token() const {
    return progress_token_;
}

const std::string& RequestContext::request_id() const {
    return request_id_;
}

void RequestContext::report_progress(double progress, const std::string& message) {
    // No progress token, skip notification
    if (!has_progress_token()) {
        return;
    }

    // Clamp progress to 0-100 range
    progress = std::clamp(progress, 0.0, 100.0);

    // Build the progress notification
    nlohmann::json notification = {
        {"jsonrpc", "2.0"},
        {"method", "notifications/progress"},
        {"params", {
            {"progressToken", *progress_token_},
            {"progress", progress}
        }}
    };

    // Add optional message if provided
    if (!message.empty()) {
        notification["params"]["message"] = message;
    }

    // Send via transport with newline delimiter (stdio spec)
    std::string serialized = notification.dump() + "\n";
    transport_.send(serialized);
}

transport::Transport& RequestContext::transport() {
    return transport_;
}

} // namespace server
} // namespace mcpp
