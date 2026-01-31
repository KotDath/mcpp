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

#ifndef MCPP_SERVER_TASK_MANAGER_H
#define MCPP_SERVER_TASK_MANAGER_H

#include <chrono>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

namespace mcpp {
namespace server {

/**
 * @brief Task status enumeration
 *
 * Represents the current state of a long-running task.
 * States follow the MCP tasks specification (experimental).
 */
enum class TaskStatus {
    Working,        ///< Task is currently executing
    InputRequired,  ///< Task needs user input to proceed
    Completed,      ///< Task completed successfully
    Failed,         ///< Task failed
    Cancelled       ///< Task was cancelled
};

/**
 * @brief Convert TaskStatus to string representation
 *
 * @param status The task status
 * @return String representation matching MCP protocol
 */
inline const char* to_string(TaskStatus status) {
    switch (status) {
        case TaskStatus::Working: return "working";
        case TaskStatus::InputRequired: return "input_required";
        case TaskStatus::Completed: return "completed";
        case TaskStatus::Failed: return "failed";
        case TaskStatus::Cancelled: return "cancelled";
    }
    return "unknown";
}

/**
 * @brief Task metadata and state
 *
 * Stores all information about a long-running task including
 * its status, timestamps, and optional configuration.
 */
struct Task {
    std::string task_id;                          ///< Unique task identifier
    TaskStatus status;                            ///< Current status
    std::optional<std::string> status_message;    ///< Optional human-readable status
    std::string created_at;                       ///< ISO 8601 creation timestamp
    std::string last_updated_at;                  ///< ISO 8601 last update timestamp
    std::optional<uint64_t> ttl_ms;               ///< TTL in milliseconds (null = unlimited)
    std::optional<uint64_t> poll_interval_ms;     ///< Suggested poll interval for clients

    /**
     * @brief Construct a Task with current timestamp
     *
     * @param id Unique task identifier
     * @param s Initial task status
     * @param ttl Optional TTL in milliseconds
     */
    Task(std::string id, TaskStatus s, std::optional<uint64_t> ttl = std::nullopt);
};

/**
 * @brief Task lifecycle manager for long-running operations
 *
 * TaskManager provides CRUD operations for tasks following the MCP
 * tasks specification (experimental). Tasks support:
 *
 * - Unique ID generation (UUID-like format)
 * - Status tracking with state transition validation
 * - TTL-based expiration
 * - Result storage for completed tasks
 * - Thread-safe operations via mutex protection
 *
 * State transitions:
 * - Working/InputRequired -> Any state (including terminal states)
 * - Completed/Failed/Cancelled -> Terminal (no transitions allowed)
 *
 * Typical usage:
 * ```cpp
 * TaskManager manager;
 *
 * // Create a new task
 * std::string task_id = manager.create_task(60000); // 60 second TTL
 *
 * // Update task status
 * manager.update_status(task_id, TaskStatus::Completed, "Done!");
 *
 * // Set and retrieve result
 * manager.set_result(task_id, {{"output", "result"}});
 * auto result = manager.get_result(task_id);
 *
 * // List tasks with pagination
 * auto page = manager.list_tasks();
 * for (const auto& task : page.items) {
 *     std::cout << task.task_id << ": " << to_string(task.status) << std::endl;
 * }
 *
 * // Clean up expired tasks
 * size_t cleaned = manager.cleanup_expired();
 * ```
 */
class TaskManager {
public:
    /**
     * @brief Default constructor
     */
    TaskManager() = default;

    /**
     * @brief Default destructor
     */
    ~TaskManager() = default;

    // Non-copyable, non-movable
    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;
    TaskManager(TaskManager&&) = delete;
    TaskManager& operator=(TaskManager&&) = delete;

    /**
     * @brief Paginated result wrapper
     *
     * Generic container for paginated results with optional cursor.
     */
    template<typename T>
    struct PaginatedResult {
        std::vector<T> items;
        std::optional<std::string> nextCursor;
        bool has_more() const noexcept { return nextCursor.has_value(); }
    };

    /**
     * @brief Create a new task
     *
     * Generates a unique task ID and creates a task in "working" status.
     *
     * @param ttl_ms Optional TTL in milliseconds (nullopt = unlimited)
     * @param poll_interval_ms Optional suggested poll interval
     * @return New task ID
     */
    std::string create_task(
        std::optional<uint64_t> ttl_ms = std::nullopt,
        std::optional<uint64_t> poll_interval_ms = std::nullopt
    );

    /**
     * @brief Get task metadata by ID
     *
     * @param task_id Task identifier
     * @return Task metadata, or nullopt if not found
     */
    std::optional<Task> get_task(const std::string& task_id) const;

    /**
     * @brief Update task status
     *
     * Validates state transitions. Terminal states (Completed, Failed, Cancelled)
     * cannot transition to other states.
     *
     * @param task_id Task identifier
     * @param status New status
     * @param message Optional status message
     * @return true if update succeeded, false if invalid transition or task not found
     */
    bool update_status(
        const std::string& task_id,
        TaskStatus status,
        const std::optional<std::string>& message = std::nullopt
    );

    /**
     * @brief Set the result for a completed task
     *
     * @param task_id Task identifier
     * @param result Task result as JSON
     * @return true if set successfully, false if task not found
     */
    bool set_result(const std::string& task_id, const nlohmann::json& result);

    /**
     * @brief Get the result for a completed task
     *
     * @param task_id Task identifier
     * @return Task result, or nullopt if not found or no result set
     */
    std::optional<nlohmann::json> get_result(const std::string& task_id) const;

    /**
     * @brief Cancel a task
     *
     * Transitions task to Cancelled status if not already terminal.
     *
     * @param task_id Task identifier
     * @return true if cancelled, false if not found or already terminal
     */
    bool cancel_task(const std::string& task_id);

    /**
     * @brief Delete a task
     *
     * Removes task from storage. Use for cleanup.
     *
     * @param task_id Task identifier
     * @return true if deleted, false if not found
     */
    bool delete_task(const std::string& task_id);

    /**
     * @brief List tasks with pagination
     *
     * Returns a paginated list of tasks. The nextCursor can be used
     * to retrieve subsequent pages.
     *
     * @param cursor Optional pagination cursor
     * @return Paginated result of tasks
     */
    PaginatedResult<Task> list_tasks(
        const std::optional<std::string>& cursor = std::nullopt
    ) const;

    /**
     * @brief Clean up expired tasks
     *
     * Removes tasks whose TTL has expired. Should be called periodically.
     *
     * @return Number of tasks cleaned up
     */
    size_t cleanup_expired();

private:
    /// Page size for task listing
    static constexpr size_t PAGE_SIZE = 50;

    /// Generate a unique task ID (UUID-like format)
    std::string generate_task_id() const;

    /// Get current ISO 8601 timestamp
    static std::string get_timestamp();

    /// Check if task has expired based on TTL
    bool is_expired(const Task& task) const;

    /// Tasks storage
    std::unordered_map<std::string, Task> tasks_;

    /// Task results storage
    std::unordered_map<std::string, nlohmann::json> results_;

    /// Mutex for thread-safe access
    mutable std::mutex mutex_;
};

} // namespace server
} // namespace mcpp

#endif // MCPP_SERVER_TASK_MANAGER_H
