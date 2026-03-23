#pragma once

#include <string>

#include "json.h"
#include "weather_gen.h"

class JsonObject;

auto load_weather_settings( const JsonObject &jo ) -> void;
auto reset_weather_settings() -> void;
auto has_weather_setting( const std::string &id ) -> bool;
auto get_weather_setting( const std::string &id ) -> const weather_generator &; // *NOPAD*
