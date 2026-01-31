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

#ifndef MCPP_CLIENT_FUTURE_WRAPPER_H
#define MCPP_CLIENT_FUTURE_WRAPPER_H

#include <chrono>
#include <exception>
#include <future>
#include <functional>
#include <memory>

#include "mcpp/core/error.h"

namespace mcpp::client {

/**
 * @brief Builder for creating std::future from callback-based async APIs
 *
 * FutureBuilder provides a template-based utility for converting callback-style
 * asynchronous APIs into std::future-based blocking APIs. This enables ergonomic
 * synchronous code while keeping the callback-based foundation (required for
 * streaming and non-blocking I/O).
 *
 * The key pattern: create a shared_ptr<promise> to ensure the promise lives
 * until the callback is invoked, even if the FutureBuilder goes out of scope.
 * The shared_ptr is captured by value in the lambda callbacks.
 *
 * Usage:
 *   auto future = FutureBuilder<InitializeResult>::wrap(
 *       [&](auto on_success, auto on_error) {
 *           client.initialize(params, on_success, on_error);
 *       }
 *   );
 *   auto result = future.get();  // Blocks until complete
 *
 * Thread safety: Not thread-safe. Use external synchronization if needed.
 *
 * @tparam T The type of value produced by the async operation
 */
template<typename T>
class FutureBuilder {
public:
    /**
     * @brief Shared pointer to promise type
     *
     * Using shared_ptr ensures the promise lives until callbacks complete.
     */
    using PromisePtr = std::shared_ptr<std::promise<T>>;

    /**
     * @brief Future type for this builder
     */
    using Future = std::future<T>;

    /**
     * @brief Success callback type
     *
     * Callback invoked with the result value on success.
     */
    using SuccessCallback = std::function<void(const T&)>;

    /**
     * @brief Error callback type
     *
     * Callback invoked with a JsonRpcError on failure.
     */
    using ErrorCallback = std::function<void(const core::JsonRpcError&)>;

    /**
     * @brief Create a promise/future pair
     *
     * Creates a new promise and its associated future.
     * The promise is wrapped in a shared_ptr for lifetime management.
     *
     * @return A pair containing the promise pointer and future
     */
    static std::pair<PromisePtr, Future> create() {
        auto promise = std::make_shared<std::promise<T>>();
        auto future = promise->get_future();
        return {std::move(promise), std::move(future)};
    }

    /**
     * @brief Wrap a callback-based async function, returning a future
     *
     * Converts a callback-style async API into a future-based API.
     * The async_fn is invoked with two callbacks: success and error.
     *
     * Exception safety:
     * - If async_fn throws, the exception is stored in the promise
     * - If callbacks throw when setting value, the exception is silently caught
     *   (promise may already be satisfied)
     *
     * @param async_fn Function that takes (on_success, on_error) callbacks
     * @return A future that will contain the result or exception
     */
    template<typename AsyncFn>
    static Future wrap(AsyncFn&& async_fn) {
        auto [promise, future] = create();

        // Call async function with callbacks that set the promise
        try {
            async_fn(
                [promise](const T& result) {
                    try {
                        promise->set_value(result);
                    } catch (...) {
                        // Promise already satisfied or other error - silently ignore
                    }
                },
                [promise](const core::JsonRpcError& error) {
                    try {
                        promise->set_exception(std::make_exception_ptr(
                            std::runtime_error(error.message)
                        ));
                    } catch (...) {
                        // Promise already satisfied - silently ignore
                    }
                }
            );
        } catch (...) {
            // If async_fn itself throws, store the exception
            promise->set_exception(std::current_exception());
        }

        return future;
    }

    /**
     * @brief Wait for future with timeout
     *
     * Blocks until the future is ready or the timeout expires.
     * Throws std::runtime_error on timeout.
     *
     * @param future The future to wait for
     * @param timeout Maximum duration to wait
     * @return The result value
     * @throws std::runtime_error if timeout expires
     * @throws Any exception stored in the future
     */
    static T with_timeout(Future&& future, std::chrono::milliseconds timeout) {
        auto status = future.wait_for(timeout);
        if (status == std::future_status::timeout) {
            throw std::runtime_error("Request timeout");
        }
        return future.get();
    }
};

} // namespace mcpp::client

#endif // MCPP_CLIENT_FUTURE_WRAPPER_H
