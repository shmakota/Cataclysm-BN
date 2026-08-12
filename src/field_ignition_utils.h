#pragma once

#include <algorithm>

#include "calendar.h"
#include "field_type.h"

/// Fuel puddles need to ignite into a self-sustaining fire rather than a
/// one-turn spark, otherwise connected spills do not reliably carry flame.
inline auto fuel_field_fire_intensity( const int fuel_intensity ) -> int
{
    if( fuel_intensity <= 0 ) {
        return 1;
    }
    return std::min( fd_fire.obj().get_max_intensity(), std::max( 2, fuel_intensity ) );
}

inline auto fuel_field_fire_age( const int fuel_intensity ) -> time_duration
{
    if( fuel_intensity <= 0 ) {
        return 10_minutes;
    }
    return -10_minutes * fuel_intensity;
}
