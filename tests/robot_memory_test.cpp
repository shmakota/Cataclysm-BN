#include "avatar.h"
#include "calendar.h"
#include "cata_utility.h"
#include "catch/catch.hpp"
#include "crafting.h"
#include "digital_media.h"
#include "inventory.h"
#include "item.h"
#include "item_contents.h"
#include "item_handling_util.h"
#include "itype.h"
#include "json.h"
#include "location_vector.h"
#include "map/map.h"
#include "player_helpers.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "requirements.h"
#include "state_helpers.h"
#include "text_snippets.h"

#include <algorithm>
#include <sstream>
#include <vector>

TEST_CASE("android_disassembly_always_recovers_memory", "[robot_memory][disassembly]") {
    clear_all_state();
    const auto cleanup = on_out_of_scope([]() { clear_all_state(); });
    auto who = avatar{};
    clear_character(who);
    const auto id = itype_id(GENERATE(
        "broken_nursebot", "broken_nursebot_defective", "broken_grocerybot",
        "broken_grocerybot_busted", "bot_nursebot", "bot_grocerybot", "bot_grocerybot_busted"));
    auto& android = who.i_add(item::spawn(id));
    const auto nurse =
        id == itype_id("broken_nursebot") || id == itype_id("broken_nursebot_defective")
        || id == itype_id("bot_nursebot");
    const auto expected_memory = itype_id(
        nurse ? "digital_nursebot_memory" : "digital_grocerybot_memory");
    android.set_damage(3000);
    const auto& uncraft = recipe_dictionary::get_uncraft(id);
    const auto disassembly_requirements = uncraft.disassembly_requirements();
    for (const auto& alternatives : disassembly_requirements.get_tools()) {
        const auto& requirement = alternatives.front();
        auto tool = item::spawn(requirement.type);
        if (tool->ammo_default()) { tool->ammo_set(tool->ammo_default(), 10000); }
        who.i_add(std::move(tool));
    }
    SECTION("salvaged android") {}
    SECTION("constructed android with an SD card") {
        android.get_components().push_back(item::spawn("sd_memory_card"));
    }
    crafting::complete_disassemble(who, iuse_location(android, 1), who.bub_pos());
    const auto drops = get_map().i_at(who.bub_pos());
    const auto drives = std::ranges::count_if(drops, [](const auto* item) {
        return item->typeId() == itype_id("usb_drive");
    });
    REQUIRE(drives == 1);
    const auto found = std::ranges::find_if(drops, [](const auto* item) {
        return item->typeId() == itype_id("usb_drive");
    });
    const auto& drive = **found;
    REQUIRE_FALSE(drive.contents.empty());
    for (const auto* file : drive.contents.all_items_top()) {
        CHECK(file->typeId() == expected_memory);
        CHECK(is_digital_file(*file));
        CHECK(SNIPPET.has_snippet_with_id(file->snip_id));
    }
    CHECK(std::ranges::none_of(drops, [](const auto* item) {
        return item->typeId() == itype_id("sd_memory_card");
    }));
}

TEST_CASE("android_construction_requires_removable_storage", "[robot_memory][crafting]") {
    const auto& recipe = recipe_id(GENERATE("bot_nursebot", "bot_grocerybot")).obj();
    const auto components = recipe.simple_requirements().get_components();
    const auto requirements = requirement_data({}, {}, components);
    auto inv = inventory{};
    auto owned = std::vector<detached_ptr<item>>{};
    for (const auto& alternatives : components) {
        if (std::ranges::any_of(alternatives, [](const auto& component) {
                return component.type == itype_id("usb_drive")
                    || component.type == itype_id("sd_memory_card");
            })) {
            continue;
        }
        const auto& component = alternatives.front();
        for (auto count = 0; count < component.count; ++count) {
            owned.push_back(item::spawn(component.type));
            inv.add_item(*owned.back(), false);
        }
    }
    CHECK_FALSE(requirements.can_make_with_inventory(inv, recipe.get_component_filter()));
    const auto storage = item::spawn(GENERATE("usb_drive", "sd_memory_card"));
    inv.add_item(*storage, false);
    CHECK(requirements.can_make_with_inventory(inv, recipe.get_component_filter()));
}

TEST_CASE("existing_android_saves_gain_memory_recovery", "[robot_memory][savegame]") {
    auto android = item::spawn(GENERATE("broken_nursebot", "broken_grocerybot"));
    const auto expected_group = android->get_var("digital_robot_memory");
    REQUIRE_FALSE(expected_group.empty());
    android->erase_var("digital_robot_memory");
    auto output = std::ostringstream{};
    auto writer = JsonOut(output);
    android->serialize(writer);
    auto input = std::istringstream(output.str());
    auto parser = JsonIn(input);
    const auto loaded = item::spawn(parser);
    CHECK(loaded->get_var("digital_robot_memory") == expected_group);
}
