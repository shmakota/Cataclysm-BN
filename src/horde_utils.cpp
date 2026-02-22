#include "horde_utils.h"

#include <algorithm>
#include <ranges>
#include <tuple>

#include "coordinates.h"
#include "mongroup.h"
#include "overmap.h"
#include "overmapbuffer.h"

namespace
{

using cata::horde::horde_info;
using cata::horde::horde_query_options;
using cata::horde::horde_spawn_options;
using cata::horde::horde_move_options;

struct sm_bounds {
    tripoint min = tripoint_zero;
    tripoint max = tripoint_zero;
};

auto normalize_bounds( const horde_query_options &opts ) -> horde_query_options
{
    auto min_x = std::min( opts.min.x, opts.max.x );
    auto min_y = std::min( opts.min.y, opts.max.y );
    auto min_z = std::min( opts.min.z, opts.max.z );
    auto max_x = std::max( opts.min.x, opts.max.x );
    auto max_y = std::max( opts.min.y, opts.max.y );
    auto max_z = std::max( opts.min.z, opts.max.z );
    return horde_query_options{
        .min = tripoint( min_x, min_y, min_z ),
        .max = tripoint( max_x, max_y, max_z )
    };
}

auto to_abs_sm( const tripoint &pos_omt ) -> tripoint_abs_sm
{
    return project_to<coords::sm>( tripoint_abs_omt( pos_omt ) );
}

auto omt_bounds_to_sm_bounds( const horde_query_options &opts ) -> sm_bounds
{
    auto sm_min = to_abs_sm( opts.min );
    auto sm_max = to_abs_sm( opts.max );
    auto sm_max_raw = sm_max.raw() + tripoint_south_east;
    return sm_bounds{
        .min = sm_min.raw(),
        .max = sm_max_raw
    };
}

auto horde_target_abs_sm( const mongroup &group,
                          const tripoint_abs_sm &pos_abs_sm ) -> tripoint_abs_sm
{
    auto abs_om = point_abs_om{};
    auto local_sm = tripoint_om_sm{};
    std::tie( abs_om, local_sm ) = project_remain<coords::om>( pos_abs_sm );
    return project_combine( abs_om, group.target );
}

auto make_horde_info( const mongroup &group,
                      const tripoint_abs_sm &pos_abs_sm ) -> horde_info
{
    auto pos_abs_omt = project_to<coords::omt>( pos_abs_sm );
    auto target_abs_sm = horde_target_abs_sm( group, pos_abs_sm );
    auto target_abs_omt = project_to<coords::omt>( target_abs_sm );
    auto monster_count = static_cast<int>( group.monsters.size() );
    return horde_info{
        .type = group.type,
        .pos_sm = pos_abs_sm.raw(),
        .pos_omt = pos_abs_omt.raw(),
        .target_sm = target_abs_sm.raw(),
        .target_omt = target_abs_omt.raw(),
        .population = static_cast<int>( group.population ),
        .radius = static_cast<int>( group.radius ),
        .interest = group.interest,
        .dying = group.dying,
        .diffuse = group.diffuse,
        .horde_behaviour = group.horde_behaviour,
        .monster_count = monster_count,
        .avg_speed = group.avg_speed(),
        .is_safe = group.is_safe()
    };
}

auto pos_overmap( const tripoint_abs_sm &pos_abs_sm ) -> point_abs_om
{
    auto abs_om = point_abs_om{};
    auto local_sm = tripoint_om_sm{};
    std::tie( abs_om, local_sm ) = project_remain<coords::om>( pos_abs_sm );
    return abs_om;
}

} // namespace

auto cata::horde::get_hordes_in_bounds( const horde_query_options &opts )
-> std::vector<horde_info>
{
    auto normalized = normalize_bounds( opts );
    auto sm_range = omt_bounds_to_sm_bounds( normalized );
    auto results = std::vector<horde_info>{};
    auto z_range = std::views::iota( sm_range.min.z, sm_range.max.z + 1 );
    std::ranges::for_each( z_range, [&]( const auto z ) {
        auto y_range = std::views::iota( sm_range.min.y, sm_range.max.y + 1 );
        std::ranges::for_each( y_range, [&]( const auto y ) {
            auto x_range = std::views::iota( sm_range.min.x, sm_range.max.x + 1 );
            std::ranges::for_each( x_range, [&]( const auto x ) {
                auto pos_abs_sm = tripoint_abs_sm( tripoint( x, y, z ) );
                auto groups = overmap_buffer.groups_at( pos_abs_sm );
                std::ranges::for_each( groups, [&]( auto *group ) {
                    if( group == nullptr || !group->horde ) {
                        return;
                    }
                    results.push_back( make_horde_info( *group, pos_abs_sm ) );
                } );
            } );
        } );
    } );
    return results;
}

auto cata::horde::add_horde( const horde_spawn_options &opts )
-> std::expected<horde_info, std::string>
{
    if( !opts.type.is_valid() ) {
        return std::unexpected( "invalid monster group id" );
    }

    auto pos_abs_sm = to_abs_sm( opts.pos );
    auto pos_abs_om = point_abs_om{};
    auto pos_local_sm = tripoint_om_sm{};
    std::tie( pos_abs_om, pos_local_sm ) = project_remain<coords::om>( pos_abs_sm );

    auto target_omt = opts.target == tripoint_min ? opts.pos : opts.target;
    auto target_abs_sm = to_abs_sm( target_omt );
    auto target_abs_om = point_abs_om{};
    auto target_local_sm = tripoint_om_sm{};
    std::tie( target_abs_om, target_local_sm ) = project_remain<coords::om>( target_abs_sm );
    if( target_abs_om != pos_abs_om ) {
        return std::unexpected( "target must be in the same overmap as the horde" );
    }

    auto radius = std::max( 1, opts.radius );
    auto population = std::max( 0, opts.population );
    auto group = mongroup( opts.type, pos_local_sm,
                           static_cast<unsigned int>( radius ),
                           static_cast<unsigned int>( population ) );
    group.horde = true;
    group.dying = opts.dying;
    group.diffuse = opts.diffuse;
    group.horde_behaviour = opts.horde_behaviour;
    group.target = target_local_sm;
    group.interest = std::clamp( opts.interest, 0, 100 );

    overmap_buffer.add_mon_group( pos_abs_om, group );

    return make_horde_info( group, pos_abs_sm );
}

auto cata::horde::remove_hordes_at( const tripoint &pos ) -> int
{
    auto pos_abs_omt = tripoint_abs_omt( pos );
    auto pos_abs_sm = project_to<coords::sm>( pos_abs_omt );
    auto removed = 0;
    auto groups = overmap_buffer.monsters_at( pos_abs_omt );
    std::ranges::for_each( groups, [&]( auto *group ) {
        if( group == nullptr || !group->horde ) {
            return;
        }
        group->clear();
        ++removed;
    } );
    if( removed > 0 ) {
        overmap_buffer.process_mongroups();
    }
    return removed;
}

auto cata::horde::move_hordes_at( const horde_move_options &opts ) -> int
{
    auto from_abs_omt = tripoint_abs_omt( opts.from );
    auto from_abs_sm = project_to<coords::sm>( from_abs_omt );
    auto from_abs_om = point_abs_om{};
    auto from_projected = project_remain<coords::om>( from_abs_sm );
    from_abs_om = from_projected.quotient;

    auto to_abs_omt = tripoint_abs_omt( opts.to );
    auto to_abs_sm = project_to<coords::sm>( to_abs_omt );
    auto to_abs_om = point_abs_om{};
    auto to_projected = project_remain<coords::om>( to_abs_sm );
    to_abs_om = to_projected.quotient;

    auto groups = overmap_buffer.monsters_at( from_abs_omt );
    auto moved_groups = std::vector<mongroup>{};
    std::ranges::for_each( groups, [&]( auto *group ) {
        if( group == nullptr || !group->horde ) {
            return;
        }
        auto group_abs_sm = project_combine( from_abs_om, group->pos );
        auto offset = group_abs_sm.raw() - from_abs_sm.raw();
        auto new_abs_sm = tripoint_abs_sm( to_abs_sm.raw() + offset );
        auto new_projected = project_remain<coords::om>( new_abs_sm );
        auto new_local_sm = new_projected.remainder_tripoint;
        auto delta = group->target.raw() - group->pos.raw();
        auto moved = *group;
        moved.pos = new_local_sm;
        moved.target = tripoint_om_sm( new_local_sm.raw() + delta );
        moved_groups.push_back( moved );
        group->clear();
    } );

    if( moved_groups.empty() ) {
        return 0;
    }

    overmap_buffer.process_mongroups();

    std::ranges::for_each( moved_groups, [&]( const auto &group ) {
        overmap_buffer.add_mon_group( to_abs_om, group );
    } );

    return static_cast<int>( moved_groups.size() );
}
