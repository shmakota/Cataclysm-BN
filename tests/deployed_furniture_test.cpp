#include "catch/catch.hpp"

#include <ranges>

#include "calendar.h"
#include "debug.h"
#include "game.h"
#include "item.h"
#include "item_stack.h"
#include "map.h"
#include "map_utils.h"
#include "state_helpers.h"
#include "type_id.h"

TEST_CASE( "take_down_deployed_furniture_keeps_furniture_vars", "[iexamine][deployed_furniture]" )
{
    clear_all_state();
    auto &here = get_map();
    const auto pos = tripoint( 60, 60, 0 );
    here.ter_set( pos, ter_id( "t_floor" ) );
    here.furn_set( pos, furn_id( "f_cardboard_box" ) );
    here.i_clear( pos );
    here.furn_vars( pos )->set( "test_var", "kept" );

    const auto debug_msg = capture_debugmsg_during( [&]() {
        take_down_deployed_furniture( pos, pos );
    } );

    CHECK( debug_msg.empty() );
    CHECK( here.furn( pos ) == f_null );
    auto dropped_items = here.i_at( pos );
    REQUIRE( dropped_items.size() == 1 );
    const auto &dropped_item = dropped_items.only_item();
    CHECK( dropped_item.typeId() == itype_id( "box_large" ) );
    CHECK( dropped_item.get_var( "test_var" ) == "kept" );
}

TEST_CASE( "take_down_deployed_furniture_puts_fit_items_inside_box",
           "[iexamine][deployed_furniture]" )
{
    clear_all_state();
    auto &here = get_map();
    const auto pos = tripoint( 60, 60, 0 );
    here.ter_set( pos, ter_id( "t_floor" ) );
    here.furn_set( pos, furn_id( "f_cardboard_box" ) );
    here.i_clear( pos );
    here.spawn_item( pos, itype_id( "concrete" ), 1, 1000, calendar::turn, 0 );

    take_down_deployed_furniture( pos, pos );

    auto items = here.i_at( pos );
    REQUIRE( items.size() == 1 );
    const auto box_it = std::ranges::find_if( items, []( const item * const it ) {
        return it->typeId() == itype_id( "box_large" );
    } );
    REQUIRE( box_it != items.end() );
    const item &box = **box_it;
    REQUIRE( box.contents.num_item_stacks() == 1 );
    REQUIRE( box.contents.front().is_container() );
    REQUIRE( box.contents.front().contents.num_item_stacks() == 1 );
    CHECK( box.contents.front().contents.front().typeId() == itype_id( "concrete" ) );
    CHECK( box.contents.front().contents.front().charges == 1000 );
}
