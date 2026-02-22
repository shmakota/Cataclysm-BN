#pragma once

#include <expected>
#include <string>
#include <vector>

#include "point.h"
#include "type_id.h"

namespace cata::horde
{

/// Summary data for a single horde (positions are in absolute coords).
struct horde_info {
    mongroup_id type;
    tripoint pos_sm = tripoint_zero;
    tripoint pos_omt = tripoint_zero;
    tripoint target_sm = tripoint_zero;
    tripoint target_omt = tripoint_zero;
    int population = 0;
    int radius = 0;
    int interest = 0;
    bool dying = false;
    bool diffuse = false;
    std::string horde_behaviour;
    int monster_count = 0;
    float avg_speed = 0.0f;
    bool is_safe = false;
};

/// Options for spawning a horde (pos/target are absolute OMT coords).
struct horde_spawn_options {
    mongroup_id type;
    tripoint pos = tripoint_zero;
    tripoint target = tripoint_min;
    int population = 1;
    int radius = 1;
    int interest = 0;
    bool dying = false;
    bool diffuse = false;
    std::string horde_behaviour;
};

/// Options for querying hordes in an area (absolute OMT coords).
struct horde_query_options {
    tripoint min = tripoint_zero;
    tripoint max = tripoint_zero;
};

/// Options for moving hordes (absolute OMT coords).
struct horde_move_options {
    tripoint from = tripoint_zero;
    tripoint to = tripoint_zero;
};

auto get_hordes_in_bounds( const horde_query_options &opts ) -> std::vector<horde_info>;
auto add_horde( const horde_spawn_options &opts ) -> std::expected<horde_info, std::string>;
auto remove_hordes_at( const tripoint &pos ) -> int;
auto move_hordes_at( const horde_move_options &opts ) -> int;

} // namespace cata::horde
