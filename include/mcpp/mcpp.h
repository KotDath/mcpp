#pragma once

/**
 * @file mcpp.h
 * @brief Main header file for the MCP C++ library
 *
 * This file includes all the core components of the MCP library.
 * Users should include this file to get access to all MCP functionality.
 */

// Core components
#include "mcpp/core/json_rpc_message.h"
#include "mcpp/core/protocol_version.h"

// Transport layer
#include "mcpp/transport/transport.h"
#include "mcpp/transport/stdio_transport.h"

// Model layer
#include "mcpp/model/request.h"
#include "mcpp/model/response.h"
#include "mcpp/model/notification.h"

// Utilities
#include "mcpp/utils/logger.h"
#include "mcpp/utils/error.h"

// Library version
#define MCPP_VERSION_MAJOR 1
#define MCPP_VERSION_MINOR 0
#define MCPP_VERSION_PATCH 0
#define MCPP_VERSION "1.0.0"

namespace mcpp {

/**
 * @brief Initialize the MCP library
 *
 * This function must be called before using any other MCP functionality.
 * It initializes the logger and sets up the library.
 */
inline void initialize() {
    utils::Logger::initialize();
}

/**
 * @brief Get library version
 */
inline const char* get_version() {
    return MCPP_VERSION;
}

} // namespace mcpp