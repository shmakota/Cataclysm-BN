#include "catalua_network.h"

#include <algorithm>
#include <curl/curl.h>
#include <ranges>

namespace
{

/// RAII wrapper for CURL handles.
struct curl_handle {
    explicit curl_handle( CURL *handle_in ) : handle( handle_in ) {}
    curl_handle( const curl_handle & ) = delete;
    curl_handle &operator=( const curl_handle & ) = delete;
    curl_handle( curl_handle && ) noexcept = default;
    curl_handle &operator=( curl_handle && ) noexcept = default;
    ~curl_handle() {
        if( handle ) {
            curl_easy_cleanup( handle );
        }
    }

    CURL *handle = nullptr;
};

/// RAII wrapper for CURL header lists.
struct curl_header_list {
    curl_header_list() = default;
    curl_header_list( const curl_header_list & ) = delete;
    curl_header_list &operator=( const curl_header_list & ) = delete;
    ~curl_header_list() {
        if( list ) {
            curl_slist_free_all( list );
        }
    }

    curl_slist *list = nullptr;
};

auto write_callback( char *ptr, size_t size, size_t nmemb, void *userdata ) -> size_t {
    const auto total = size * nmemb;
    if( userdata != nullptr ) {
        auto *target = static_cast<std::string *>( userdata );
        target->append( ptr, total );
    }
    return total;
}

} // namespace

auto cata::detail::perform_http_request( const http_request_options &opts ) -> std::expected<http_response, std::string>
{
    auto *raw_handle = curl_easy_init();
    if( raw_handle == nullptr ) {
        return std::unexpected{ "failed to initialize curl" };
    }

    auto handle = curl_handle{ raw_handle };
    auto headers = curl_header_list{};

    curl_easy_setopt( handle.handle, CURLOPT_URL, opts.url.c_str() );
    curl_easy_setopt( handle.handle, CURLOPT_WRITEFUNCTION, write_callback );
    curl_easy_setopt( handle.handle, CURLOPT_TIMEOUT_MS, static_cast<long>( opts.timeout.count() ) );

    if( opts.insecure ) {
        curl_easy_setopt( handle.handle, CURLOPT_SSL_VERIFYPEER, 0L );
        curl_easy_setopt( handle.handle, CURLOPT_SSL_VERIFYHOST, 0L );
    }

    std::ranges::for_each( opts.headers, [&]( const std::string &header_line ) {
        headers.list = curl_slist_append( headers.list, header_line.c_str() );
    } );

    if( headers.list != nullptr ) {
        curl_easy_setopt( handle.handle, CURLOPT_HTTPHEADER, headers.list );
    }

    if( opts.method != "GET" ) {
        curl_easy_setopt( handle.handle, CURLOPT_CUSTOMREQUEST, opts.method.c_str() );
    }

    if( !opts.body.empty() ) {
        curl_easy_setopt( handle.handle, CURLOPT_POSTFIELDS, opts.body.c_str() );
        curl_easy_setopt( handle.handle, CURLOPT_POSTFIELDSIZE, static_cast<long>( opts.body.size() ) );
    }

    auto response = http_response{};
    curl_easy_setopt( handle.handle, CURLOPT_WRITEDATA, &response.body );

    const auto result = curl_easy_perform( handle.handle );
    if( result != CURLE_OK ) {
        return std::unexpected{ curl_easy_strerror( result ) };
    }

    long code = 0;
    curl_easy_getinfo( handle.handle, CURLINFO_RESPONSE_CODE, &code );
    response.status = static_cast<int>( code );

    return response;
}
