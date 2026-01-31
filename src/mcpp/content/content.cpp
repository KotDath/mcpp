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

#include "mcpp/content/content.h"

#include "mcpp/client/sampling.h"
#include "mcpp/server/resource_registry.h"

#include <variant>

namespace mcpp::content {

namespace {

// ============================================================================
// Annotations Conversion
// ============================================================================

/**
 * @brief Convert Annotations to JSON
 */
nlohmann::json annotations_to_json(const Annotations& a) {
    nlohmann::json j;

    if (a.audience.has_value()) {
        j["audience"] = *a.audience;
    }
    if (a.priority.has_value()) {
        j["priority"] = *a.priority;
    }
    if (a.last_modified.has_value()) {
        j["lastModified"] = *a.last_modified;
    }

    return j;
}

/**
 * @brief Parse Annotations from JSON
 */
std::optional<Annotations> annotations_from_json(const nlohmann::json& j) {
    Annotations annotations;

    // Parse audience (optional, must be array of strings if present)
    if (j.contains("audience")) {
        if (!j["audience"].is_array()) {
            return std::nullopt;
        }
        std::vector<std::string> audience;
        for (const auto& item : j["audience"]) {
            if (!item.is_string()) {
                return std::nullopt;
            }
            audience.push_back(item.get<std::string>());
        }
        annotations.audience = std::move(audience);
    }

    // Parse priority (optional, must be number 0-1 if present)
    if (j.contains("priority")) {
        if (!j["priority"].is_number()) {
            return std::nullopt;
        }
        double priority = j["priority"].get<double>();
        if (priority < 0.0 || priority > 1.0) {
            return std::nullopt;
        }
        annotations.priority = priority;
    }

    // Parse lastModified (optional, must be string if present)
    if (j.contains("lastModified")) {
        if (!j["lastModified"].is_string()) {
            return std::nullopt;
        }
        annotations.last_modified = j["lastModified"].get<std::string>();
    }

    return annotations;
}

} // namespace

// ============================================================================
// ContentBlock JSON Conversion
// ============================================================================

nlohmann::json content_to_json(const ::mcpp::client::ContentBlock& content) {
    return std::visit([](const auto& c) -> nlohmann::json {
        nlohmann::json j;
        j["type"] = c.type;

        using ContentType = std::decay_t<decltype(c)>;

        if constexpr (std::is_same_v<ContentType, ::mcpp::client::TextContent>) {
            j["text"] = c.text;

        } else if constexpr (std::is_same_v<ContentType, ImageContent>) {
            j["data"] = c.data;
            j["mimeType"] = c.mime_type;
            if (c.annotations.has_value()) {
                j["annotations"] = annotations_to_json(*c.annotations);
            }

        } else if constexpr (std::is_same_v<ContentType, AudioContent>) {
            j["data"] = c.data;
            j["mimeType"] = c.mime_type;
            if (c.annotations.has_value()) {
                j["annotations"] = annotations_to_json(*c.annotations);
            }

        } else if constexpr (std::is_same_v<ContentType, ResourceLink>) {
            j["uri"] = c.uri;
            if (c.annotations.has_value()) {
                j["annotations"] = annotations_to_json(*c.annotations);
            }

        } else if constexpr (std::is_same_v<ContentType, EmbeddedResource>) {
            // Serialize the embedded resource
            j["uri"] = c.resource.uri;
            if (c.resource.mime_type.has_value()) {
                j["mimeType"] = *c.resource.mime_type;
            }
            if (c.resource.is_text) {
                j["text"] = c.resource.text;
            } else {
                j["blob"] = c.resource.blob;
            }
            if (c.annotations.has_value()) {
                j["annotations"] = annotations_to_json(*c.annotations);
            }

        } else if constexpr (std::is_same_v<ContentType, ::mcpp::client::ToolUseContent>) {
            j["id"] = c.id;
            j["name"] = c.name;
            j["arguments"] = c.arguments;

        } else if constexpr (std::is_same_v<ContentType, ::mcpp::client::ToolResultContent>) {
            j["tool_use_id"] = c.tool_use_id;
            if (c.content.has_value()) {
                j["content"] = *c.content;
            }
            if (c.is_error.has_value()) {
                j["isError"] = *c.is_error;
            }
        }

        return j;
    }, content);
}

std::optional<::mcpp::client::ContentBlock> content_from_json(const nlohmann::json& j) {
    if (!j.contains("type") || !j["type"].is_string()) {
        return std::nullopt;
    }

    std::string type = j["type"].get<std::string>();

    // Parse annotations if present (shared across all content types)
    std::optional<Annotations> annotations;
    if (j.contains("annotations")) {
        annotations = annotations_from_json(j["annotations"]);
        if (!annotations) {
            return std::nullopt;
        }
    }

    if (type == "text") {
        if (!j.contains("text") || !j["text"].is_string()) {
            return std::nullopt;
        }
        ::mcpp::client::TextContent content;
        content.type = "text";
        content.text = j["text"].get<std::string>();
        return content;

    } else if (type == "image") {
        if (!j.contains("data") || !j["data"].is_string()) {
            return std::nullopt;
        }
        if (!j.contains("mimeType") || !j["mimeType"].is_string()) {
            return std::nullopt;
        }
        ImageContent content;
        content.type = "image";
        content.data = j["data"].get<std::string>();
        content.mime_type = j["mimeType"].get<std::string>();
        content.annotations = std::move(annotations);
        return content;

    } else if (type == "audio") {
        if (!j.contains("data") || !j["data"].is_string()) {
            return std::nullopt;
        }
        if (!j.contains("mimeType") || !j["mimeType"].is_string()) {
            return std::nullopt;
        }
        AudioContent content;
        content.type = "audio";
        content.data = j["data"].get<std::string>();
        content.mime_type = j["mimeType"].get<std::string>();
        content.annotations = std::move(annotations);
        return content;

    } else if (type == "resource") {
        if (!j.contains("uri") || !j["uri"].is_string()) {
            return std::nullopt;
        }
        ResourceLink content;
        content.type = "resource";
        content.uri = j["uri"].get<std::string>();
        content.annotations = std::move(annotations);
        return content;

    } else if (type == "embedded") {
        if (!j.contains("uri") || !j["uri"].is_string()) {
            return std::nullopt;
        }
        EmbeddedResource content;
        content.type = "embedded";
        content.resource.uri = j["uri"].get<std::string>();

        // Parse optional mime_type
        if (j.contains("mimeType") && j["mimeType"].is_string()) {
            content.resource.mime_type = j["mimeType"].get<std::string>();
        }

        // Parse either text or blob content
        if (j.contains("text") && j["text"].is_string()) {
            content.resource.is_text = true;
            content.resource.text = j["text"].get<std::string>();
        } else if (j.contains("blob") && j["blob"].is_string()) {
            content.resource.is_text = false;
            content.resource.blob = j["blob"].get<std::string>();
        } else {
            // Embedded resource must have either text or blob
            return std::nullopt;
        }

        content.annotations = std::move(annotations);
        return content;

    } else if (type == "tool_use") {
        ::mcpp::client::ToolUseContent content;
        content.type = "tool_use";
        if (j.contains("id") && j["id"].is_string()) {
            content.id = j["id"].get<std::string>();
        }
        if (j.contains("name") && j["name"].is_string()) {
            content.name = j["name"].get<std::string>();
        }
        if (j.contains("arguments")) {
            content.arguments = j["arguments"];
        }
        return content;

    } else if (type == "tool_result") {
        ::mcpp::client::ToolResultContent content;
        content.type = "tool_result";
        if (j.contains("tool_use_id") && j["tool_use_id"].is_string()) {
            content.tool_use_id = j["tool_use_id"].get<std::string>();
        }
        if (j.contains("content") && j["content"].is_string()) {
            content.content = j["content"].get<std::string>();
        }
        if (j.contains("isError") && j["isError"].is_boolean()) {
            content.is_error = j["isError"].get<bool>();
        }
        return content;
    }

    // Unknown content type
    return std::nullopt;
}

} // namespace mcpp::content
