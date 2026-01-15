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

/// Performs a blocking HTTP request with the provided options.
auto perform_http_request( const http_request_options &opts ) -> std::expected<http_response, std::string>;

} // namespace cata::detail
