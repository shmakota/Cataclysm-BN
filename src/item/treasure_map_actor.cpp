#include "treasure_map_actor.h"

#include "coordinates.h"
#include "enums.h"
#include "item.h"
#include "json.h"
#include "map/mapbuffer.h"
#include "map/mapdata.h"
#include "messages.h"
#include "overmap/overmap.h"
#include "overmap/overmap_ui.h"
#include "overmap/overmapbuffer.h"
#include "player.h"
#include "rng.h"
#include "string_formatter.h"
#include "translations.h"
#include "trap.h"

#include <climits>
#include <memory>
#include <optional>
#include <string>

auto treasure_map_actor::clone() const -> std::unique_ptr<iuse_actor> {
    return std::make_unique<treasure_map_actor>(*this);
}

auto treasure_map_actor::load(const JsonObject& obj) -> void {
    radius = obj.get_int("radius", radius);
    one_time = obj.get_bool("one_time", false);
    if (obj.has_string("furniture")) {
        target_furniture = furn_str_id(obj.get_string("furniture"));
    }
    if (obj.has_string("terrain")) { target_terrain = ter_str_id(obj.get_string("terrain")); }
    if (target_furniture.has_value() == target_terrain.has_value()) {
        obj.throw_error("specify exactly one of furniture or terrain");
    }
    if (radius < 1) { obj.throw_error("radius must be positive"); }
    for (const JsonValue candidate : obj.get_array("overmap_terrain")) {
        candidate_overmap_terrain.push_back(candidate.get_string());
    }
    if (candidate_overmap_terrain.empty()) {
        obj.throw_error("overmap_terrain must contain at least one candidate");
    }
}

auto treasure_map_actor::use(
    player& p, item& it, bool /*active*/, const tripoint_bub_ms& /*pos*/) const -> int {
    constexpr auto target_var = "treasure_target_abs_ms";
    auto redemption_key = std::string{};
    if (one_time) {
        if (!it.has_var("DIGITAL_FILE_ID")) {
            it.set_var("DIGITAL_FILE_ID",
                       std::to_string(rng_bits()) + "-" + std::to_string(rng_bits()) + "-"
                           + std::to_string(rng_bits()) + "-" + std::to_string(rng_bits()));
        }
        redemption_key = "dead_drop_location_redeemed_" + it.get_var("DIGITAL_FILE_ID");
        if (!p.get_value(redemption_key).empty()) {
            p.add_msg_if_player(m_info, _("This dead drop location has already been used."));
            it.detach();
            return 0;
        }
    }
    auto target = std::optional<tripoint_abs_ms>{};
    if (it.has_var(target_var)) {
        target = it.get_var(target_var, tripoint_abs_ms::zero());
    } else {
        auto params = omt_find_params{};
        params.search_range = {1, radius};
        params.search_layers = {0, 0};
        params.existing_only = false;
        for (const auto& candidate : candidate_overmap_terrain) {
            params.types.emplace_back(candidate, ot_match_type::type);
        }

        auto& overmaps = get_overmapbuffer(p.get_dimension());
        const auto omt = overmaps.find_closest(p.abs_omt_pos(), params);
        if (omt == overmap::invalid_tripoint) {
            p.add_msg_if_player(m_info, _("The map does not show a reachable treasure site."));
            return 0;
        }

        auto& buffer = p.get_mapbuffer();
        const auto view =
            buffer.get_abs_omt_view(omt, {.mode = mapbuffer_lookup_mode::load_or_generate});
        if (!view || !view->is_complete()) {
            p.add_msg_if_player(m_info, _("You cannot make out the location on the map."));
            return 0;
        }

        auto placement = std::optional<tripoint_abs_ms>{};
        auto nearest_target_distance = INT_MAX;
        auto nearest_placement_distance = INT_MAX;
        for (auto y = 0; y < 2; ++y) {
            for (auto x = 0; x < 2; ++x) {
                const auto submap_view = view->get_submap_view(point_omt_sm(x, y));
                if (!submap_view) { continue; }
                for (const auto& local : submap_view->tiles()) {
                    const auto tile = submap_view->tile(local);
                    const auto distance = square_dist(p.abs_pos(), tile.abs_pos());
                    const auto matches =
                        target_furniture
                            ? tile.get_furn() == target_furniture->id()
                            : tile.get_ter() == target_terrain->id();
                    if (matches && distance < nearest_target_distance) {
                        target = tile.abs_pos();
                        nearest_target_distance = distance;
                    }
                    if (tile.get_furn() == f_null && tile.passable_ter_furn()
                        && tile.get_trap() == tr_null && tile.get_items().empty()
                        && distance < nearest_placement_distance) {
                        placement = tile.abs_pos();
                        nearest_placement_distance = distance;
                    }
                }
            }
        }

        if (!target && placement) {
            const auto lookup = mapbuffer_lookup_options{
                .mode = mapbuffer_lookup_mode::resident_only};
            const auto placed =
                target_furniture
                    ? buffer.set_furn(*placement, target_furniture->id(), lookup)
                    : buffer.set_ter(*placement, target_terrain->id(), lookup);
            if (placed) { target = placement; }
        }
        if (!target) {
            p.add_msg_if_player(
                m_info, _("The treasure site has no suitable place for the marked tile."));
            return 0;
        }
        it.set_var(target_var, *target);
    }

    const auto omt = project_to<coords::omt>(*target);
    const auto local = project_remain<coords::omt>(*target).remainder;
    auto& overmaps = get_overmapbuffer(p.get_dimension());
    overmaps.reveal(omt, 0);
    if (!it.has_var("treasure_site_marked")) {
        auto note = std::string{};
        if (one_time && target_furniture) { note += "SPRITE:" + target_furniture->str() + ";"; }
        note += one_time ? string_format(_("Dead drop at tile (%d, %d)"), local.x(), local.y())
                         : string_format(_("Treasure at tile (%d, %d)"), local.x(), local.y());
        if (overmaps.has_note(omt)) { note += "; " + overmaps.note(omt); }
        overmaps.add_note(omt, note);
        it.set_var("treasure_site_marked", 1);
    }
    if (one_time) {
        p.add_msg_if_player(
            m_good, _("The file marks a dead drop at tile (%d, %d) in this area."), local.x(),
            local.y());
    } else {
        p.add_msg_if_player(
            m_good, _("The map marks a specific tile at (%d, %d) in this area."), local.x(),
            local.y());
    }
    ui::omap::choose_point(omt);
    if (one_time) {
        p.set_value(redemption_key, "1");
        it.detach();
    }
    return 0;
}
