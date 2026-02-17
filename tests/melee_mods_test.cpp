#include "catch/catch.hpp"

#include "damage.h"
#include "item.h"
#include "ret_val.h"
#include "type_id.h"

TEST_CASE( "melee_mod_overkill_folds_damage", "[melee][gunmod]" )
{
    item &katana = *item::spawn_temporary( "katana" );
    detached_ptr<item> mod = item::spawn( "melee_mod_overkill" );

    REQUIRE( katana.is_gunmod_compatible( *mod ).success() );
    katana.put_in( std::move( mod ) );

    const std::map<std::string, attack_statblock> attacks = katana.get_attacks();
    REQUIRE( attacks.contains( "DEFAULT" ) );
    const attack_statblock &attack = attacks.find( "DEFAULT" )->second;

    CHECK( attack.damage.type_damage( DT_CUT ) == Approx( 9000034.0f ) );
    CHECK( attack.damage.type_damage( DT_ELECTRIC ) == Approx( 9000000.0f ) );
    CHECK( attack.to_hit == 0 );
}
