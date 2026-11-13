#pragma once

#include "iuse.h"
#include "type_id.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

/// Reveals an exact terrain or furniture tile, creating one in a suitable area if needed.
class treasure_map_actor final: public iuse_actor {
public:
    treasure_map_actor(): iuse_actor("treasure_map") {}

    auto clone() const -> std::unique_ptr<iuse_actor> override;
    auto load(const JsonObject& obj) -> void override;
    auto use(player& p, item& it, bool /*active*/, const tripoint_bub_ms& /*pos*/) const
        -> int override;

private:
    int radius = 30;
    bool one_time = false;
    std::optional<furn_str_id> target_furniture;
    std::optional<ter_str_id> target_terrain;
    std::vector<std::string> candidate_overmap_terrain;
};
