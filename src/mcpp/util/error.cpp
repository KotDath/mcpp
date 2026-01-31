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

#include "mcpp/util/error.h"

#include <sstream>

namespace mcpp::util {

//==============================================================================
// ServiceError
//==============================================================================

ServiceError::ServiceError(
    std::string_view message,
    std::optional<int> code,
    std::map<std::string, std::string> context
)
    : std::runtime_error("")
    , message_(message)
    , code_(std::move(code))
    , context_(std::move(context))
{
}

ServiceError::~ServiceError() = default;

std::optional<int> ServiceError::code() const noexcept {
    return code_;
}

const std::map<std::string, std::string>& ServiceError::context() const noexcept {
    return context_;
}

const char* ServiceError::what() const noexcept {
    try {
        std::ostringstream oss;
        oss << message_;
        if (code_) {
            oss << " (code: " << *code_ << ")";
        }
        if (!context_.empty()) {
            oss << " [";
            bool first = true;
            for (const auto& [key, value] : context_) {
                if (!first) {
                    oss << ", ";
                }
                oss << key << "=" << value;
                first = false;
            }
            oss << "]";
        }
        what_buffer_ = oss.str();
        return what_buffer_.c_str();
    } catch (...) {
        return message_.c_str();
    }
}

//==============================================================================
// TransportError
//==============================================================================

TransportError::TransportError(
    std::string_view message,
    std::optional<int> code,
    std::optional<std::string> transport_type,
    std::map<std::string, std::string> context
)
    : ServiceError(message, code, std::move(context))
    , transport_type_(std::move(transport_type))
{
    // Add transport_type to context if provided
    if (transport_type_) {
        context_["transport_type"] = *transport_type_;
    }
}

std::optional<std::string> TransportError::transport_type() const noexcept {
    return transport_type_;
}

//==============================================================================
// ProtocolError
//==============================================================================

ProtocolError::ProtocolError(
    std::string_view message,
    std::optional<int> code,
    std::optional<std::string> protocol_version,
    std::map<std::string, std::string> context
)
    : ServiceError(message, code, std::move(context))
    , protocol_version_(std::move(protocol_version))
{
    // Add protocol_version to context if provided
    if (protocol_version_) {
        context_["protocol_version"] = *protocol_version_;
    }
}

std::optional<std::string> ProtocolError::protocol_version() const noexcept {
    return protocol_version_;
}

//==============================================================================
// RequestError
//==============================================================================

RequestError::RequestError(
    std::string_view message,
    std::optional<int> code,
    std::optional<std::string> method,
    std::optional<std::string> request_id,
    std::map<std::string, std::string> context
)
    : ServiceError(message, code, std::move(context))
    , method_(std::move(method))
    , request_id_(std::move(request_id))
{
    // Add method and request_id to context if provided
    if (method_) {
        context_["method"] = *method_;
    }
    if (request_id_) {
        context_["request_id"] = *request_id_;
    }
}

std::optional<std::string> RequestError::method() const noexcept {
    return method_;
}

std::optional<std::string> RequestError::request_id() const noexcept {
    return request_id_;
}

} // namespace mcpp::util
