#include "catch/catch.hpp"

#include <algorithm>

#include "avatar.h"
#include "character_functions.h"
#include "item.h"

TEST_CASE( "revolver_reload_option", "[reload],[reload_option],[gun]" )
{
    const time_point bday = calendar::start_of_cataclysm;
    avatar dummy;

    detached_ptr<item> det = item::spawn( "sw_619", bday, 0 );
    item &gun = *det;
    dummy.i_add( std::move( det ) );
    det = item::spawn( "38_special", bday, gun.ammo_capacity() );
    item &ammo = *det;
    dummy.i_add( std::move( det ) );
    REQUIRE( gun.has_flag( flag_id( "RELOAD_ONE" ) ) );
    REQUIRE( gun.ammo_remaining() == 0 );

    const item_reload_option gun_option( &dummy, &gun, &gun, ammo );
    REQUIRE( gun_option.qty() == 1 );

    det = item::spawn( "38_speedloader", bday, 0 );
    item &speedloader = *det;
    dummy.i_add( std::move( det ) );
    REQUIRE( speedloader.ammo_remaining() == 0 );

    const item_reload_option speedloader_option( &dummy, &speedloader, &speedloader,
            ammo );
    CHECK( speedloader_option.qty() == speedloader.ammo_capacity() );

    speedloader.put_in( item::spawn( ammo ) );
    const item_reload_option gun_speedloader_option( &dummy, &gun, &gun,
            speedloader );
    CHECK( gun_speedloader_option.qty() == speedloader.ammo_capacity() );
}

TEST_CASE( "magazine_reload_option", "[reload],[reload_option],[gun]" )
{
    const time_point bday = calendar::start_of_cataclysm;
    avatar dummy;

    detached_ptr<item> det = item::spawn( "glockmag", bday, 0 );
    item &magazine = *det;
    dummy.i_add( std::move( det ) );
    det = item::spawn( "9mm", bday, magazine.ammo_capacity() );
    item &ammo = *det;
    dummy.i_add( std::move( det ) );

    const item_reload_option magazine_option( &dummy, &magazine, &magazine,
            ammo );
    CHECK( magazine_option.qty() == magazine.ammo_capacity() );

    magazine.put_in( item::spawn( ammo ) );
    det = item::spawn( "glock_19", bday, 0 );
    item &gun = *det;
    dummy.i_add( std::move( det ) );
    const item_reload_option gun_option( &dummy, &gun, &gun, magazine );
    CHECK( gun_option.qty() == 1 );
}

TEST_CASE( "belt_reload_option", "[reload],[reload_option],[gun]" )
{
    const time_point bday = calendar::start_of_cataclysm;
    avatar dummy;
    dummy.set_body();

    detached_ptr<item> det = item::spawn( "belt308", bday, 0 );
    item &belt = *det;
    dummy.i_add( std::move( det ) );
    det = item::spawn( "308", bday, belt.ammo_capacity() );
    item &ammo = *det;
    dummy.i_add( std::move( det ) );
    dummy.i_add( item::spawn( "ammolink308", bday, belt.ammo_capacity() ) );
    // Belt is populated with "charges" rounds by the item constructor.
    belt.ammo_unset();

    REQUIRE( belt.ammo_remaining() == 0 );
    const item_reload_option belt_option( &dummy, &belt, &belt, ammo );
    CHECK( belt_option.qty() == belt.ammo_capacity() );

    belt.put_in( item::spawn( ammo ) );
    det = item::spawn( "m134", bday, 0 );
    item &gun = *det;
    dummy.i_add( std::move( det ) );

    const item_reload_option gun_option( &dummy, &gun, &gun, belt );

    CHECK( gun_option.qty() == 1 );
}

TEST_CASE( "canteen_reload_option", "[reload],[reload_option],[liquid]" )
{
    avatar dummy;

    detached_ptr<item> det = item::spawn( "water_clean", calendar::start_of_cataclysm, 2 );
    item &water = *det;
    dummy.i_add( std::move( det ) );
    det = item::spawn( "bottle_plastic" );
    item &bottle = *det;
    dummy.i_add( std::move( det ) );

    const item_reload_option bottle_option( &dummy, &bottle, &bottle, water );
    CHECK( bottle_option.qty() == bottle.get_remaining_capacity_for_liquid( water, true ) );

    // Add water to bottle?
    bottle.fill_with( item::spawn( water ), 2 );
    det = item::spawn( "2lcanteen" );
    item &canteen = *det;
    dummy.i_add( std::move( det ) );

    const item_reload_option canteen_option( &dummy, &canteen, &canteen,
            bottle );

    CHECK( canteen_option.qty() == 2 );
}

TEST_CASE( "container_reload_option_with_non_comestible_solid",
           "[reload],[reload_option],[container]" )
{
    const time_point bday = calendar::start_of_cataclysm;
    avatar dummy;

    auto det = item::spawn( "test_jug_plastic", bday, 0 );
    auto &jug = *det;
    dummy.i_add( std::move( det ) );

    det = item::spawn( "test_solid_stack", bday, 10 );
    auto &solid_stack = *det;
    dummy.i_add( std::move( det ) );

    REQUIRE( dummy.can_reload( jug, solid_stack.typeId() ) );

    const item_reload_option jug_option( &dummy, &jug, &jug, solid_stack );
    CHECK( jug_option.qty() == 10 );

    const auto ok = jug.reload( dummy, solid_stack, jug_option.qty() );
    REQUIRE( ok );
    REQUIRE( jug.contents.num_item_stacks() == 1 );
    REQUIRE( jug.contents.front().typeId() == solid_stack.typeId() );
    REQUIRE( jug.contents.front().charges == 10 );
}

TEST_CASE( "container_reload_option_with_non_stackable_solid",
           "[reload],[reload_option],[container]" )
{
    const time_point bday = calendar::start_of_cataclysm;
    avatar dummy;

    auto det = item::spawn( "test_jug_plastic", bday, 0 );
    auto &jug = *det;
    dummy.i_add( std::move( det ) );

    auto first = item::spawn( "test_solid_item", bday, 0 );
    auto &first_solid = *first;
    dummy.i_add( std::move( first ) );

    auto second = item::spawn( "test_solid_item", bday, 0 );
    auto &second_solid = *second;
    dummy.i_add( std::move( second ) );

    REQUIRE( dummy.can_reload( jug, first_solid.typeId() ) );

    const item_reload_option first_option( &dummy, &jug, &jug, first_solid );
    CHECK( first_option.qty() == 1 );
    REQUIRE( jug.reload( dummy, first_solid, first_option.qty() ) );
    REQUIRE( jug.contents.num_item_stacks() == 1 );
    REQUIRE( jug.contents.front().typeId() == first_solid.typeId() );

    REQUIRE( dummy.can_reload( jug, second_solid.typeId() ) );

    const item_reload_option second_option( &dummy, &jug, &jug, second_solid );
    CHECK( second_option.qty() == 1 );
    REQUIRE( jug.reload( dummy, second_solid, second_option.qty() ) );
    REQUIRE( jug.contents.num_item_stacks() == 2 );
    REQUIRE( std::ranges::all_of( jug.contents.all_items_top(), []( const item * const entry ) {
        return entry->typeId() == itype_id( "test_solid_item" );
    } ) );
}

TEST_CASE( "watertight_container_reload_option_accepts_mixed_solids",
           "[reload],[reload_option],[container]" )
{
    const auto bday = calendar::start_of_cataclysm;
    auto dummy = avatar{};

    auto det = item::spawn( "bottle_plastic", bday, 0 );
    auto &bottle = *det;
    dummy.i_add( std::move( det ) );

    det = item::spawn( "aspirin", bday, 20 );
    auto &aspirin = *det;
    dummy.i_add( std::move( det ) );

    const auto aspirin_option = item_reload_option( &dummy, &bottle, &bottle, aspirin );
    REQUIRE( bottle.reload( dummy, aspirin, aspirin_option.qty() ) );
    REQUIRE( bottle.contents.num_item_stacks() == 1 );
    REQUIRE( bottle.contents.front().typeId() == itype_id( "aspirin" ) );

    det = item::spawn( "antifungal", bday, 5 );
    auto &antifungal = *det;
    dummy.i_add( std::move( det ) );

    REQUIRE( dummy.can_reload( bottle, antifungal.typeId() ) );

    const auto antifungal_option = item_reload_option( &dummy, &bottle, &bottle, antifungal );
    CHECK( antifungal_option.qty() > 0 );
    REQUIRE( bottle.reload( dummy, antifungal, antifungal_option.qty() ) );
    REQUIRE( bottle.contents.num_item_stacks() == 2 );
}

TEST_CASE( "full_container_reload_option_does_not_consume_solid_source",
           "[reload],[reload_option],[container]" )
{
    const auto bday = calendar::start_of_cataclysm;
    auto dummy = avatar{};

    auto det = item::spawn( "bottle_plastic_small", bday, 0 );
    auto &bottle = *det;
    dummy.i_add( std::move( det ) );

    det = item::spawn( "aspirin", bday, 200 );
    auto &filling_aspirin = *det;
    dummy.i_add( std::move( det ) );

    const auto full_option = item_reload_option( &dummy, &bottle, &bottle, filling_aspirin );
    REQUIRE( bottle.reload( dummy, filling_aspirin, full_option.qty() ) );
    REQUIRE( bottle.is_container_full() );
    const auto contained_charges = bottle.contents.front().charges;

    det = item::spawn( "aspirin", bday, 20 );
    auto &extra_aspirin = *det;
    dummy.i_add( std::move( det ) );
    const auto source_charges = extra_aspirin.charges;

    const auto blocked_option = item_reload_option( &dummy, &bottle, &bottle, extra_aspirin );
    CHECK( blocked_option.qty() == 0 );
    const auto sources = character_funcs::find_ammo_items_or_mags( dummy, bottle, true, -1 );
    CHECK_FALSE( std::ranges::contains( sources, &extra_aspirin ) );
    CHECK_FALSE( bottle.reload( dummy, extra_aspirin, 1 ) );
    CHECK( extra_aspirin.charges == source_charges );
    REQUIRE( bottle.contents.num_item_stacks() == 1 );
    CHECK( bottle.contents.front().charges == contained_charges );
}

TEST_CASE( "empty_container_is_not_solid_reload_source", "[reload],[reload_option],[container]" )
{
    const time_point bday = calendar::start_of_cataclysm;
    avatar dummy;

    auto det = item::spawn( "test_jug_plastic", bday, 0 );
    auto &jug = *det;
    dummy.i_add( std::move( det ) );

    auto empty = item::spawn( "test_jug_plastic", bday, 0 );
    auto &empty_jug = *empty;
    dummy.i_add( std::move( empty ) );

    auto solid = item::spawn( "test_solid_item", bday, 0 );
    auto &solid_item = *solid;
    dummy.i_add( std::move( solid ) );

    const auto sources = character_funcs::find_ammo_items_or_mags( dummy, jug, true, -1 );
    REQUIRE( std::ranges::contains( sources, &solid_item ) );
    REQUIRE( !std::ranges::contains( sources, &empty_jug ) );
}

TEST_CASE( "multi_item_container_is_not_solid_reload_source",
           "[reload],[reload_option],[container]" )
{
    const time_point bday = calendar::start_of_cataclysm;
    avatar dummy;

    auto det = item::spawn( "test_jug_plastic", bday, 0 );
    auto &jug = *det;
    dummy.i_add( std::move( det ) );

    auto source = item::spawn( "test_jug_plastic", bday, 0 );
    auto &source_jug = *source;
    source_jug.put_in( item::spawn( "test_solid_item", bday, 0 ) );
    source_jug.put_in( item::spawn( "test_solid_item", bday, 0 ) );
    dummy.i_add( std::move( source ) );

    const auto sources = character_funcs::find_ammo_items_or_mags( dummy, jug, true, -1 );
    REQUIRE( !std::ranges::contains( sources, &source_jug ) );
}

TEST_CASE( "single_item_container_solid_reload_uses_direct_detach",
           "[reload],[reload_option],[container]" )
{
    const time_point bday = calendar::start_of_cataclysm;
    avatar dummy;

    auto det = item::spawn( "test_jug_plastic", bday, 0 );
    auto &jug = *det;
    dummy.i_add( std::move( det ) );

    auto source = item::spawn( "test_jug_plastic", bday, 0 );
    auto &source_jug = *source;
    source_jug.put_in( item::spawn( "test_solid_item", bday, 0 ) );
    dummy.i_add( std::move( source ) );

    REQUIRE( dummy.can_reload( jug, source_jug.contents.front().typeId() ) );

    const item_reload_option option( &dummy, &jug, &jug, source_jug );
    CHECK( option.qty() == 1 );
    REQUIRE( jug.reload( dummy, source_jug, option.qty() ) );
    REQUIRE( jug.contents.num_item_stacks() == 1 );
    REQUIRE( jug.contents.front().typeId() == itype_id( "test_solid_item" ) );
    REQUIRE( source_jug.contents.empty() );
}
