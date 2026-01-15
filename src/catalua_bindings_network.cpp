#include "catalua_bindings.h"
#include "catalua_bindings_utils.h"
#include "catalua_luna.h"
#include "catalua_luna_doc.h"
#include "catalua_network.h"

#include <map>
#include <ranges>
#include <string>
#include <vector>

namespace
{

/// Build a strongly typed request description from a Lua table.
auto make_request_options( const sol::table &opts ) -> std::expected<cata::detail::http_request_options, std::string>
{
    auto options = cata::detail::http_request_options{};

    const auto url_value = opts.get<sol::optional<std::string>>( "url" );
    if( !url_value || url_value->empty() ) {
        return std::unexpected{ "network.request requires a non-empty 'url' field" };
    }
    options.url = *url_value;

    if( const auto method_value = opts.get<sol::optional<std::string>>( "method" ); method_value && !method_value->empty() ) {
        options.method = *method_value;
    }

    if( const auto body_value = opts.get<sol::optional<std::string>>( "body" ); body_value ) {
        options.body = *body_value;
    }

    if( const auto timeout_value = opts.get<sol::optional<int>>( "timeout_ms" ); timeout_value && *timeout_value > 0 ) {
        options.timeout = std::chrono::milliseconds( *timeout_value );
    }

    if( const auto insecure_value = opts.get<sol::optional<bool>>( "insecure" ); insecure_value ) {
        options.insecure = *insecure_value;
    }

    if( const auto header_list = opts.get<sol::optional<std::vector<std::string>>>( "headers" ); header_list ) {
        std::ranges::for_each( *header_list, [&]( const auto &entry ) {
            options.headers.push_back( entry );
        } );
    } else if( const auto header_map = opts.get<sol::optional<sol::as_table_t<std::map<std::string, std::string>>>>( "headers" );
               header_map ) {
        std::ranges::for_each( header_map->value(), [&]( const auto &entry ) {
            options.headers.push_back( entry.first + ": " + entry.second );
        } );
    }

    return options;
}

} // namespace

void cata::detail::reg_network_api( sol::state &lua )
{
    DOC( "General networking helpers for Lua scripts." );
    auto lib = luna::begin_lib( lua, "network" );

    luna::set_fx( lib, "request",
    []( sol::this_state lua_state, sol::table opts ) {
        sol::state_view lua( lua_state );
        sol::table result = lua.create_table();

        if( !opts.valid() ) {
            result["ok"] = false;
            result["error"] = "network.request requires an options table";
            return result;
        }

        auto request_options = make_request_options( opts );
        if( !request_options ) {
            result["ok"] = false;
            result["error"] = request_options.error();
            return result;
        }

        auto response = perform_http_request( *request_options );
        if( !response ) {
            result["ok"] = false;
            result["error"] = response.error();
            return result;
        }

        result["ok"] = true;
        result["status"] = response->status;
        result["body"] = response->body;
        return result;
    } );

    luna::finalize_lib( lib );
}
