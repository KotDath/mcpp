#ifndef MCPP_UTIL_RETRY_H
#define MCPP_UTIL_RETRY_H

#include <chrono>
#include <cmath>
#include <exception>
#include <functional>
#include <thread>
#include <variant>

#include "mcpp/core/error.h"

namespace mcpp::util {

/**
 * Result type for operations that may fail.
 * Combines successful value with error information.
 */
template<typename T>
class Result {
public:
    // Construct from success value
    explicit Result(T value) : data_(std::move(value)) {}

    // Construct from error
    explicit Result(const mcpp::core::JsonRpcError& error) : data_(error) {}

    // Check if result holds a value
    bool is_ok() const { return std::holds_alternative<T>(data_); }

    // Check if result holds an error
    bool is_error() const { return std::holds_alternative<mcpp::core::JsonRpcError>(data_); }

    // Get the value (undefined if is_error() == true)
    const T& value() const& { return std::get<T>(data_); }
    T& value() & { return std::get<T>(data_); }
    T&& value() && { return std::move(std::get<T>(data_)); }

    // Get the error (undefined if is_ok() == true)
    const mcpp::core::JsonRpcError& error() const& {
        return std::get<mcpp::core::JsonRpcError>(data_);
    }

private:
    std::variant<T, mcpp::core::JsonRpcError> data_;
};

/**
 * RetryPolicy<T> defines the interface for retry strategies.
 *
 * A retry policy determines:
 * - How long to wait before the next retry attempt
 * - Whether a given error should trigger a retry
 *
 * @tparam T The result type (not directly used by the policy interface,
 *           but allows for type-specific retry logic in implementations)
 */
template<typename T>
class RetryPolicy {
public:
    virtual ~RetryPolicy() = default;

    /**
     * Calculate the delay before the next retry attempt.
     *
     * @param attempt The attempt number (1-indexed, so first retry is attempt 1)
     * @return The delay to wait before the next attempt
     */
    virtual std::chrono::milliseconds next_delay(int attempt) const = 0;

    /**
     * Determine if an error should trigger a retry.
     *
     * @param e The exception that occurred
     * @return true if the operation should be retried, false otherwise
     */
    virtual bool should_retry(const std::exception& e) const = 0;
};

/**
 * ExponentialBackoff implements exponential backoff retry strategy.
 *
 * Delays increase exponentially with each attempt:
 * attempt 1: initial_delay
 * attempt 2: initial_delay * multiplier
 * attempt 3: initial_delay * multiplier^2
 * ...capped at max_delay
 *
 * Common configuration:
 * - initial_delay: 1000ms (1 second)
 * - multiplier: 2.0 (double each time)
 * - max_delay: 30000ms (30 seconds)
 *
 * Produces delays: 1s, 2s, 4s, 8s, 16s, 30s, 30s, ...
 */
class ExponentialBackoff : public RetryPolicy<void> {
public:
    /**
     * Construct an ExponentialBackoff policy.
     *
     * @param initial_delay Initial delay before first retry (default: 1000ms)
     * @param multiplier Multiplier for each subsequent attempt (default: 2.0)
     * @param max_delay Maximum delay between attempts (default: 30000ms)
     */
    explicit ExponentialBackoff(
        std::chrono::milliseconds initial_delay = std::chrono::milliseconds(1000),
        double multiplier = 2.0,
        std::chrono::milliseconds max_delay = std::chrono::milliseconds(30000)
    ) : initial_delay_(initial_delay.count())
      , multiplier_(multiplier)
      , max_delay_(max_delay.count()) {}

    /**
     * Calculate exponential backoff delay.
     * delay = min(initial_delay * multiplier^(attempt-1), max_delay)
     *
     * @param attempt The attempt number (1-indexed)
     * @return Delay in milliseconds
     */
    std::chrono::milliseconds next_delay(int attempt) const override {
        // Calculate: initial_delay * (multiplier ^ (attempt - 1))
        double delay = initial_delay_ * std::pow(multiplier_, attempt - 1);

        // Cap at max_delay
        if (delay > max_delay_) {
            delay = max_delay_;
        }

        return std::chrono::milliseconds(static_cast<int64_t>(delay));
    }

    /**
     * Default implementation retries all exceptions.
     * Subclasses can override for selective retry behavior.
     */
    bool should_retry(const std::exception& e) const override {
        (void)e;  // Suppress unused warning
        return true;  // Retry all errors by default
    }

private:
    double initial_delay_;  // Initial delay in milliseconds
    double multiplier_;     // Multiplier for exponential growth
    double max_delay_;      // Maximum delay in milliseconds
};

/**
 * LinearBackoff implements linear backoff retry strategy.
 *
 * Delays increase linearly with each attempt:
 * attempt 1: initial_delay
 * attempt 2: initial_delay + increment
 * attempt 3: initial_delay + 2 * increment
 * ...capped at max_delay
 *
 * Produces more predictable delay progression than exponential backoff.
 */
class LinearBackoff : public RetryPolicy<void> {
public:
    /**
     * Construct a LinearBackoff policy.
     *
     * @param initial_delay Initial delay before first retry
     * @param increment Amount to add for each subsequent attempt
     * @param max_delay Maximum delay between attempts
     */
    explicit LinearBackoff(
        std::chrono::milliseconds initial_delay = std::chrono::milliseconds(1000),
        std::chrono::milliseconds increment = std::chrono::milliseconds(1000),
        std::chrono::milliseconds max_delay = std::chrono::milliseconds(30000)
    ) : initial_delay_(initial_delay.count())
      , increment_(increment.count())
      , max_delay_(max_delay.count()) {}

    /**
     * Calculate linear backoff delay.
     * delay = min(initial_delay + (attempt - 1) * increment, max_delay)
     */
    std::chrono::milliseconds next_delay(int attempt) const override {
        double delay = initial_delay_ + (attempt - 1) * increment_;

        if (delay > max_delay_) {
            delay = max_delay_;
        }

        return std::chrono::milliseconds(static_cast<int64_t>(delay));
    }

    bool should_retry(const std::exception& e) const override {
        (void)e;
        return true;
    }

private:
    double initial_delay_;
    double increment_;
    double max_delay_;
};

/**
 * Retry with backoff for operations returning Result<T>.
 *
 * Executes the function up to max_attempts times, with configurable
 * backoff delay between attempts. Returns on first success; if all
 * attempts fail, returns the last error.
 *
 * @tparam T The success type
 * @param fn Function to execute (should return Result<T>)
 * @param policy Retry policy determining backoff and retry eligibility
 * @param max_attempts Maximum number of attempts (including first attempt)
 * @return Result<T> containing success value or last error
 */
template<typename T>
Result<T> retry_with_backoff(
    std::function<Result<T>()> fn,
    const RetryPolicy<T>& policy,
    int max_attempts = 3
) {
    Result<T> last_result = Result<T>(mcpp::core::JsonRpcError{
        mcpp::core::INTERNAL_ERROR,
        "No attempts made"
    });

    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        try {
            Result<T> result = fn();

            if (result.is_ok()) {
                return result;  // Success - return immediately
            }

            // Failure - store result for potential return
            last_result = std::move(result);

            // Don't sleep after the last attempt
            if (attempt < max_attempts - 1) {
                // Check if we should retry based on the error
                std::runtime_error dummy_error(last_result.error().message);
                if (!policy.should_retry(dummy_error)) {
                    break;  // Policy says don't retry
                }

                // Sleep before next attempt
                std::chrono::milliseconds delay = policy.next_delay(attempt + 1);
                std::this_thread::sleep_for(delay);
            }
        } catch (const std::exception& e) {
            // Convert exception to error result
            last_result = Result<T>(mcpp::core::JsonRpcError{
                mcpp::core::INTERNAL_ERROR,
                e.what()
            });

            // Don't sleep after the last attempt
            if (attempt < max_attempts - 1) {
                if (!policy.should_retry(e)) {
                    break;  // Policy says don't retry
                }

                // Sleep before next attempt
                std::chrono::milliseconds delay = policy.next_delay(attempt + 1);
                std::this_thread::sleep_for(delay);
            }
        }
    }

    return last_result;  // All attempts failed
}

/**
 * Retry with backoff for operations returning T directly (throwing on error).
 *
 * Executes the function up to max_attempts times, with configurable
 * backoff delay between attempts. Returns on first success; if all
 * attempts fail, rethrows the last exception.
 *
 * @tparam T The return type
 * @param fn Function to execute (may throw exceptions)
 * @param policy Retry policy determining backoff and retry eligibility
 * @param max_attempts Maximum number of attempts
 * @return T The result of the first successful execution
 * @throws std::runtime_error if all attempts fail
 */
template<typename T>
T retry_with_backoff_exception(
    std::function<T()> fn,
    const RetryPolicy<void>& policy,
    int max_attempts = 3
) {
    std::exception_ptr last_exception;

    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        try {
            return fn();  // Success - return immediately
        } catch (const std::exception& e) {
            last_exception = std::current_exception();

            // Don't sleep after the last attempt
            if (attempt < max_attempts - 1) {
                if (!policy.should_retry(e)) {
                    break;  // Policy says don't retry
                }

                // Sleep before next attempt
                std::chrono::milliseconds delay = policy.next_delay(attempt + 1);
                std::this_thread::sleep_for(delay);
            }
        }
    }

    // All attempts failed - rethrow the last exception
    if (last_exception) {
        std::rethrow_exception(last_exception);
    }

    throw std::runtime_error("All retry attempts failed");
}

} // namespace mcpp::util

#endif // MCPP_UTIL_RETRY_H
