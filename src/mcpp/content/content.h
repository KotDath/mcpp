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

#ifndef MCPP_CONTENT_CONTENT_H
#define MCPP_CONTENT_CONTENT_H

#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

// Include for ResourceContent definition used by EmbeddedResource
#include "mcpp/server/resource_registry.h"

namespace mcpp::content {

/**
 * @brief Annotations for content metadata
 *
 * Annotations provide optional metadata for content blocks including:
 * - audience: Target roles for this content (user, assistant, system)
 * - priority: Content priority (0-1, lower = higher priority)
 * - lastModified: ISO 8601 timestamp of last modification
 *
 * Corresponds to the Annotations type in MCP 2025-11-25 schema.
 */
struct Annotations {
    /// Optional target audience (array of roles: user, assistant, system)
    std::optional<std::vector<std::string>> audience;

    /// Optional priority (0-1, lower values indicate higher priority)
    std::optional<double> priority;

    /// Optional ISO 8601 timestamp of last modification
    std::optional<std::string> last_modified;

    /// Default constructor
    Annotations() = default;

    /// Constructor with values for convenience
    Annotations(
        std::optional<std::vector<std::string>> aud,
        std::optional<double> pri,
        std::optional<std::string> lm
    ) : audience(std::move(aud)), priority(pri), last_modified(std::move(lm)) {}
};

/**
 * @brief Image content block for multi-modal LLM support
 *
 * ImageContent represents image data in content blocks for vision-enabled LLMs.
 * The image data must be base64-encoded by the caller.
 *
 * Corresponds to the "image" content type in MCP 2025-11-25 schema.
 *
 * Supported MIME types:
 * - image/png
 * - image/jpeg
 * - image/gif
 * - image/webp
 */
struct ImageContent {
    /// Content type identifier
    std::string type = "image";

    /// Base64-encoded image bytes (caller's responsibility to encode)
    std::string data;

    /// MIME type of the image (e.g., "image/png", "image/jpeg")
    std::string mime_type;

    /// Optional annotations for content metadata
    std::optional<Annotations> annotations;
};

/**
 * @brief Audio content block for multi-modal LLM support
 *
 * AudioContent represents audio data in content blocks for audio-enabled LLMs.
 * The audio data must be base64-encoded by the caller.
 *
 * Corresponds to the "audio" content type in MCP 2025-11-25 schema.
 *
 * Supported MIME types:
 * - audio/mp3
 * - audio/wav
 * - audio/ogg
 * - audio/mpeg
 */
struct AudioContent {
    /// Content type identifier
    std::string type = "audio";

    /// Base64-encoded audio bytes (caller's responsibility to encode)
    std::string data;

    /// MIME type of the audio (e.g., "audio/mp3", "audio/wav")
    std::string mime_type;

    /// Optional annotations for content metadata
    std::optional<Annotations> annotations;
};

/**
 * @brief Resource link content block
 *
 * ResourceLink references a resource by URI without embedding its content.
 * The client can resolve the resource separately if needed.
 *
 * Corresponds to the "resource" content type in MCP 2025-11-25 schema.
 */
struct ResourceLink {
    /// Content type identifier
    std::string type = "resource";

    /// URI referencing the resource
    std::string uri;

    /// Optional annotations for content metadata
    std::optional<Annotations> annotations;
};

/**
 * @brief Embedded resource content block
 *
 * EmbeddedResource embeds resource content directly in the content block.
 * Reuses ResourceContent from resource_registry.h for the resource data.
 *
 * Corresponds to the "embedded" content type in MCP 2025-11-25 schema.
 *
 * Note: ResourceContent must be defined before use. Include resource_registry.h
 * or ensure the full definition is available when using this type.
 */
struct EmbeddedResource {
    /// Content type identifier
    std::string type = "embedded";

    /// Embedded resource content (uri, mime_type, is_text, text/blob)
    server::ResourceContent resource;

    /// Optional annotations for content metadata
    std::optional<Annotations> annotations;
};

} // namespace mcpp::content

#endif // MCPP_CONTENT_CONTENT_H
