// mcpp - MCP C++ library
// https://github.com/mcpp-project/mcpp
//
// Copyright (c) 2025 mcpp contributors
// Distributed under MIT License

#include "mcpp/server/task_manager.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>
#include <ctime>

namespace {

/**
 * @brief Parse ISO 8601 timestamp to time_t
 *
 * @param ts ISO 8601 timestamp string (e.g., "2025-01-31T12:34:56Z")
 * @return time_t value, or 0 if parsing fails
 */
std::time_t parse_iso8601(const std::string& ts) {
    std::tm tm = {};
    std::istringstream ss(ts);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    if (ss.fail()) {
        return 0;
    }
    // Use timegm to convert UTC tm to time_t (Linux-specific)
    // For portability, we assume mktime with UTC adjustment
    return timegm(&tm);
}

} // anonymous namespace

namespace mcpp {
namespace server {

// ============================================================================
// Task Constructor
// ============================================================================

Task::Task(std::string id, TaskStatus s, std::optional<uint64_t> ttl)
    : task_id(std::move(id)),
      status(s),
      created_at(TaskManager::get_timestamp()),
      last_updated_at(TaskManager::get_timestamp()),
      ttl_ms(ttl) {}

// ============================================================================
// TaskManager Implementation
// ============================================================================

std::string TaskManager::get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

std::string TaskManager::generate_task_id() const {
    // Simple UUID-like format using random_device
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 8; ++i) ss << dis(gen);
    ss << '-';
    for (int i = 0; i < 4; ++i) ss << dis(gen);
    ss << "-4";  // Version 4
    for (int i = 0; i < 3; ++i) ss << dis(gen);
    ss << '-';
    ss << std::uniform_int_distribution<>(8, 11)(gen);  // Variant
    for (int i = 0; i < 3; ++i) ss << dis(gen);
    ss << '-';
    for (int i = 0; i < 12; ++i) ss << dis(gen);

    return ss.str();
}

std::string TaskManager::create_task(
    std::optional<uint64_t> ttl_ms,
    std::optional<uint64_t> poll_interval_ms
) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string task_id = generate_task_id();
    Task task(task_id, TaskStatus::Working, ttl_ms);
    task.poll_interval_ms = poll_interval_ms;

    tasks_[task_id] = std::move(task);
    return task_id;
}

std::optional<Task> TaskManager::get_task(const std::string& task_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = tasks_.find(task_id);
    if (it == tasks_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool TaskManager::update_status(
    const std::string& id,
    TaskStatus new_status,
    const std::optional<std::string>& message
) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = tasks_.find(id);
    if (it == tasks_.end()) {
        return false;
    }

    Task& task = it->second;

    // Validate state transitions
    // Terminal states (Completed, Failed, Cancelled) cannot transition
    switch (task.status) {
        case TaskStatus::Working:
            // Can transition to any state
            break;
        case TaskStatus::InputRequired:
            // Can transition to any state
            break;
        case TaskStatus::Completed:
        case TaskStatus::Failed:
        case TaskStatus::Cancelled:
            // Terminal states - no transitions allowed
            return false;
    }

    task.status = new_status;
    task.last_updated_at = get_timestamp();
    if (message.has_value()) {
        task.status_message = message;
    }
    return true;
}

bool TaskManager::set_result(const std::string& task_id, const nlohmann::json& result) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = tasks_.find(task_id);
    if (it == tasks_.end()) {
        return false;
    }

    results_[task_id] = result;
    return true;
}

std::optional<nlohmann::json> TaskManager::get_result(const std::string& task_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = results_.find(task_id);
    if (it == results_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool TaskManager::cancel_task(const std::string& task_id) {
    // cancel_task uses update_status which handles locking
    return update_status(task_id, TaskStatus::Cancelled);
}

bool TaskManager::delete_task(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto task_it = tasks_.find(task_id);
    if (task_it == tasks_.end()) {
        return false;
    }

    tasks_.erase(task_it);
    results_.erase(task_id);
    return true;
}

TaskManager::PaginatedResult<Task> TaskManager::list_tasks(
    const std::optional<std::string>& cursor
) const {
    std::lock_guard<std::mutex> lock(mutex_);

    PaginatedResult<Task> result;

    // Get all task IDs
    std::vector<std::string> task_ids;
    task_ids.reserve(tasks_.size());
    for (const auto& pair : tasks_) {
        task_ids.push_back(pair.first);
    }

    // Sort for consistent pagination
    std::sort(task_ids.begin(), task_ids.end());

    // Find starting position from cursor
    size_t start = 0;
    if (cursor.has_value()) {
        // Cursor is the task_id to start from
        auto it = std::find(task_ids.begin(), task_ids.end(), *cursor);
        if (it != task_ids.end()) {
            start = std::distance(task_ids.begin(), it) + 1;
        }
    }

    // Extract page of tasks
    size_t end = std::min(start + PAGE_SIZE, task_ids.size());
    for (size_t i = start; i < end; ++i) {
        const auto& task = tasks_.at(task_ids[i]);
        result.items.push_back(task);
    }

    // Set next cursor if there are more items
    if (end < task_ids.size()) {
        result.nextCursor = task_ids[end];
    }

    return result;
}

bool TaskManager::is_expired(const Task& task) const {
    if (!task.ttl_ms.has_value()) {
        return false;  // No TTL = never expires
    }

    // Parse the ISO 8601 timestamp to time_t
    std::time_t created_time = parse_iso8601(task.created_at);
    if (created_time == 0) {
        return false;  // Failed to parse, assume not expired
    }

    // Calculate elapsed time since creation
    auto now = std::chrono::system_clock::now();
    auto created = std::chrono::system_clock::from_time_t(created_time);

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - created
    ).count();

    return elapsed > static_cast<int64_t>(*task.ttl_ms);
}

size_t TaskManager::cleanup_expired() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> expired_ids;
    for (const auto& pair : tasks_) {
        if (is_expired(pair.second)) {
            expired_ids.push_back(pair.first);
        }
    }

    for (const auto& id : expired_ids) {
        tasks_.erase(id);
        results_.erase(id);
    }

    return expired_ids.size();
}

} // namespace server
} // namespace mcpp
