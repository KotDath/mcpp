// mcpp - MCP C++ library
// https://github.com/mcpp-project/mcpp
//
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef MCPP_CLIENT_ELICITATION_H
#define MCPP_CLIENT_ELICITATION_H

#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

namespace mcpp::client {

// Type alias for JSON values - using nlohmann::json
using JsonValue = nlohmann::json;

// ============================================================================
// Primitive Schema for Form Mode
// ============================================================================

/**
 * @brief Primitive schema definition for elicitation form mode
 *
 * Provides a simplified JSON Schema for form field validation.
 * Per MCP 2025-11-25 spec, only top-level properties are supported
 * (no nested objects or arrays of objects).
 *
 * Supported types:
 * - "string": Text input with optional regex pattern and length constraints
 * - "number": Numeric input with min/max constraints
 * - "integer": Integer input with min/max constraints
 * - "boolean": True/false selection
 * - "array": Enum-based single or multi-select
 */
struct PrimitiveSchema {
    /**
     * @brief Data type for this field
     *
     * Valid values: "string", "number", "integer", "boolean", "array"
     */
    std::string type;  // "string", "number", "integer", "boolean", "array"

    /**
     * @brief Optional title/label for the field
     */
    std::optional<std::string> title;

    /**
     * @brief Optional description/help text for the field
     */
    std::optional<std::string> description;

    /**
     * @brief Optional default value for the field
     *
     * Named "default_value" instead of "default" to avoid keyword collision.
     */
    std::optional<std::string> default_value;

    // ------------------------------------------------------------------------
    // String type constraints
    // ------------------------------------------------------------------------

    /**
     * @brief Regex pattern for string validation
     *
     * When set, the input string must match this pattern.
     */
    std::optional<std::string> pattern;

    /**
     * @brief Minimum string length
     */
    std::optional<size_t> min_length;

    /**
     * @brief Maximum string length
     */
    std::optional<size_t> max_length;

    // ------------------------------------------------------------------------
    // Number/Integer type constraints
    // ------------------------------------------------------------------------

    /**
     * @brief Minimum numeric value
     */
    std::optional<double> minimum;

    /**
     * @brief Maximum numeric value
     */
    std::optional<double> maximum;

    // ------------------------------------------------------------------------
    // Array type constraints (enum-based select)
    // ------------------------------------------------------------------------

    /**
     * @brief Enum values for single/multi-select
     *
     * When type is "array", this provides the selectable options.
     */
    std::optional<std::vector<std::string>> enum_values;

    /**
     * @brief Whether multi-select is enabled
     *
     * When true, user can select multiple values from enum_values.
     * When false or unset, only single selection allowed.
     */
    std::optional<bool> multiselect;
};

// ============================================================================
// Elicitation Request Types
// ============================================================================

/**
 * @brief Form mode elicitation request (CLNT-04)
 *
 * The server sends this when it needs structured user input via an in-app form.
 * Form mode is synchronous - the user responds immediately via the form UI.
 *
 * Example JSON:
 * {
 *   "message": "Please enter your API credentials",
 *   "mode": "form",
 *   "requested_schema": {
 *     "api_key": {"type": "string", "description": "Your API key"},
 *     "timeout": {"type": "integer", "minimum": 1, "maximum": 300}
 *   },
 *   "required": ["api_key"]
 * }
 */
struct ElicitRequestForm {
    /**
     * @brief Message/prompt shown to the user
     *
     * Describes what information is being requested and why.
     */
    std::string message;

    /**
     * @brief Elicitation mode (always "form" for this type)
     */
    std::string mode = "form";

    /**
     * @brief Form field definitions
     *
     * Maps field names to their primitive schema definitions.
     * The user will be prompted to provide values for these fields.
     */
    std::map<std::string, PrimitiveSchema> requested_schema;

    /**
     * @brief Optional list of required field names
     *
     * When set, these fields must be filled by the user.
     * If not set, all fields are optional.
     */
    std::optional<std::vector<std::string>> required;
};

/**
 * @brief URL mode elicitation request (CLNT-05)
 *
 * The server sends this when it needs out-of-band user interaction.
 * URL mode is asynchronous - the client opens the URL and waits for
 * a completion notification from the server.
 *
 * Use cases: OAuth flows, external approvals, CAPTCHA completion, etc.
 *
 * Example JSON:
 * {
 *   "message": "Please authorize via OAuth",
 *   "mode": "url",
 *   "elicitation_id": "auth-123",
 *   "url": "https://auth.example.com/authorize",
 *   "confirm_url": "https://auth.example.com/confirm"
 * }
 */
struct ElicitRequestURL {
    /**
     * @brief Message/prompt shown to the user
     *
     * Describes why the user needs to open the URL.
     */
    std::string message;

    /**
     * @brief Elicitation mode (always "url" for this type)
     */
    std::string mode = "url";

    /**
     * @brief Unique identifier for this elicitation request
     *
     * Used to correlate the completion notification with this request.
     */
    std::string elicitation_id;

    /**
     * @brief URL to open for user interaction
     *
     * The client should open this URL in the user's browser.
     */
    std::string url;

    /**
     * @brief Optional confirmation URL
     *
     * If provided, the client can poll this URL to check completion status.
     */
    std::optional<std::string> confirm_url;
};

// ============================================================================
// Elicitation Result
// ============================================================================

/**
 * @brief User's response to an elicitation request
 *
 * The client returns this to the server after collecting user input.
 * For form mode, this includes the field values. For URL mode, this
 * is typically just the action (accept/decline/cancel).
 *
 * Example JSON (form accept):
 * {
 *   "action": "accept",
 *   "content": {"api_key": "sk-1234", "timeout": 60}
 * }
 *
 * Example JSON (decline):
 * {"action": "decline"}
 */
struct ElicitResult {
    /**
     * @brief User's action on the elicitation request
     *
     * - "accept": User provided the requested input
     * - "decline": User explicitly declined the request
     * - "cancel": User cancelled/aborted the request
     */
    std::string action;  // "accept", "decline", "cancel"

    /**
     * @brief Optional content (form field values)
     *
     * Maps field names to their values. Values can be strings, numbers,
     * booleans, or arrays of strings (for multi-select).
     *
     * Only present when action is "accept".
     */
    std::optional<std::map<std::string, std::variant<std::string, double, bool, std::vector<std::string>>>> content;

    /**
     * @brief Convert this result to JSON
     *
     * @return JSON value in MCP elicitation result format
     */
    JsonValue to_json() const;
};

// ============================================================================
// Elicitation Handler
// ============================================================================

/**
 * @brief Callback type for handling elicitation requests
 *
 * User code provides this callback to present UI and collect input.
 * The callback receives either a form or URL request and returns
 * the user's response.
 *
 * For form mode (CLNT-04):
 * - Present an in-app form with fields from requested_schema
 * - Validate required fields are present
 * - Return ElicitResult with action="accept" and content map
 *
 * For URL mode (CLNT-05):
 * - Open the URL in the user's browser
 * - Show the message to the user
 * - Store the elicitation_id for correlation
 * - Return immediately; completion comes via notification
 *
 * Example usage:
 *   client.set_elicitation_handler([](const auto& request) {
 *       if (std::holds_alternative<ElicitRequestForm>(request)) {
 *           auto form = std::get<ElicitRequestForm>(request);
 *           // Show form UI, collect input
 *           ElicitResult result;
 *           result.action = "accept";
 *           result.content = {{"api_key", user_input}};
 *           return result;
 *       } else {
 *           auto url_req = std::get<ElicitRequestURL>(request);
 *           // Open URL, return immediately
 *           open_browser(url_req.url);
 *           ElicitResult result;
 *           result.action = "accept";  // Will complete later
 *           return result;
 *       }
 *   });
 */
using ElicitationHandler = std::function<ElicitResult(const std::variant<ElicitRequestForm, ElicitRequestURL>&)>;

// ============================================================================
// Elicitation Complete Notification (URL mode)
// ============================================================================

/**
 * @brief Notification when URL mode elicitation completes
 *
 * The server sends this notification after the out-of-band interaction
 * completes. The client matches the elicitation_id to the pending request.
 *
 * Example JSON:
 * {
 *   "elicitation_id": "auth-123",
 *   "action": "accept",
 *   "content": {"auth_code": "ABC123"}
 * }
 */
struct ElicitationCompleteNotification {
    /**
     * @brief ID matching the original ElicitRequestURL
     */
    std::string elicitation_id;

    /**
     * @brief Result of the elicitation
     *
     * - "accept": User completed the flow successfully
     * - "decline": User declined the request
     * - "cancel": User cancelled the flow
     */
    std::string action;  // "accept", "decline", "cancel"

    /**
     * @brief Optional content from the out-of-band flow
     *
     * May contain results from OAuth, CAPTCHA, etc.
     */
    std::optional<nlohmann::json> content;

    /**
     * @brief Parse from JSON
     *
     * @param j JSON value to parse
     * @return Parsed notification, or nullopt if validation fails
     */
    static std::optional<ElicitationCompleteNotification> from_json(const nlohmann::json& j);
};

// ============================================================================
// Elicitation Client
// ============================================================================

/**
 * @brief Manages elicitation requests for MCP clients
 *
 * ElicitationClient handles both form mode (synchronous) and URL mode
 * (asynchronous) elicitation requests from the server.
 *
 * Form mode (CLNT-04):
 * - Server sends elicitation/create with mode="form"
 * - Client presents form UI to user
 * - User fills form and submits
 * - Client returns ElicitResult immediately
 *
 * URL mode (CLNT-05):
 * - Server sends elicitation/create with mode="url" and elicitation_id
 * - Client opens URL in browser
 * - Client stores pending request by elicitation_id
 * - Server sends notifications/elicitation/complete when done
 * - Client matches by elicitation_id and resolves pending request
 *
 * Thread safety: Not thread-safe. Use external synchronization if needed.
 *
 * Usage:
 *   ElicitationClient elicitation_client;
 *
 *   elicitation_client.set_elicitation_handler([](const auto& request) {
 *       // Present UI based on request type
 *       return collect_user_input(request);
 *   });
 *
 *   // In McpClient request handler:
 *   auto result_json = elicitation_client.handle_elicitation_create(params);
 *
 *   // In notification handler:
 *   elicitation_client.handle_elicitation_complete(params);
 */
class ElicitationClient {
public:
    ElicitationClient() = default;
    ~ElicitationClient() = default;

    // Non-copyable
    ElicitationClient(const ElicitationClient&) = delete;
    ElicitationClient& operator=(const ElicitationClient&) = delete;

    // Movable
    ElicitationClient(ElicitationClient&&) = default;
    ElicitationClient& operator=(ElicitationClient&&) = default;

    /**
     * @brief Set the handler for elicitation requests
     *
     * The handler is invoked when the server sends an elicitation/create request.
     * It should present UI to the user and return their response.
     *
     * @param handler Function that processes elicitation requests
     */
    void set_elicitation_handler(ElicitationHandler handler);

    /**
     * @brief Handle an elicitation/create request from the server
     *
     * Parses the request (form or URL mode), invokes the registered handler,
     * and returns the result as JSON.
     *
     * For form mode: Returns immediately with user's input.
     * For URL mode: Stores pending request and returns acknowledgment.
     *
     * @param params Request parameters as JSON
     * @return Result or error as JSON
     */
    nlohmann::json handle_elicitation_create(const nlohmann::json& params);

    /**
     * @brief Handle an elicitation/complete notification from the server
     *
     * Called when the server sends notifications/elicitation/complete.
     * Matches the elicitation_id to a pending URL mode request and
     * invokes the stored completion callback.
     *
     * @param params Notification parameters as JSON
     */
    void handle_elicitation_complete(const nlohmann::json& params);

private:
    /// User-provided callback for presenting elicitation UI
    ElicitationHandler handler_;

    /// Pending URL mode requests (elicitation_id -> completion callback)
    std::unordered_map<std::string, std::function<void(const ElicitResult&)>> pending_url_requests_;

    /// Mutex for thread-safe access to pending_url_requests_
    std::mutex mutex_;
};

} // namespace mcpp::client

#endif // MCPP_CLIENT_ELICITATION_H
