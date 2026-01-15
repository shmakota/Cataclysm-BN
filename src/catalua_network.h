#pragma once

#include <chrono>
#include <expected>
#include <string>
#include <vector>

namespace cata::detail
{
/// Options used when making a synchronous HTTP request for Lua.
struct http_request_options {
    std::string url;
    std::string method = "GET";
    std::string body;
    std::vector<std::string> headers;
    std::chrono::milliseconds timeout{ 5000 };
    bool insecure = false;
};

/// Results returned from an HTTP request.
struct http_response {
    int status = 0;
    std::string body;
};

/// Results returned from an async HTTP request, keyed by the originating identifier.
struct http_async_response {
    int request_id = 0;
    std::expected<http_response, std::string> response;
};

/// Performs a blocking HTTP request with the provided options.
auto perform_http_request( const http_request_options &opts ) -> std::expected<http_response, std::string>;

/// Queues a detached HTTP request and returns its identifier.
auto start_http_request_async( http_request_options opts ) -> int;

/// Collects completed async responses and removes them from the internal queue.
auto collect_http_async_responses() -> std::vector<http_async_response>;

} // namespace cata::detail
