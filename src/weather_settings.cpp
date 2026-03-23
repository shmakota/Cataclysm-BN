#include "weather_settings.h"

#include <unordered_map>
#include <utility>

#include "json.h"

namespace
{
static std::unordered_map<std::string, weather_generator> weather_settings;
} // namespace

auto load_weather_settings( const JsonObject &jo ) -> void
{
    auto style = weather_generator::load( jo );
    auto id = jo.get_string( "id" );
    if( id.empty() ) {
        jo.throw_error( "weather_settings requires non-empty id", "id" );
    }
    weather_settings[id] = std::move( style );
}

auto reset_weather_settings() -> void
{
    weather_settings.clear();
}

auto has_weather_setting( const std::string &id ) -> bool
{
    return weather_settings.contains( id );
}

auto get_weather_setting( const std::string &id ) -> const weather_generator & // *NOPAD*
{
    const auto it = weather_settings.find( id );
    if( it == weather_settings.end() ) {
        static const weather_generator fallback;
        return fallback;
    }
    return it->second;
}
