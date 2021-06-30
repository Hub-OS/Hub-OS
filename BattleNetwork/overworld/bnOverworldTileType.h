#pragma once
#include <string_view>
#include "../stx/string.h"

namespace Overworld {
  namespace TileType {
    enum TileType {
      conveyor,
      stairs,
      undefined,
    };

    inline TileType FromString(const std::string_view& type_string) {
      constexpr std::string_view conveyor_string = "Conveyor";
      constexpr std::string_view stairs_string = "Stairs";

      if (stx::insensitive_equals(type_string, conveyor_string)) {
        return TileType::conveyor;
      }
      if (stx::insensitive_equals(type_string, stairs_string)) {
        return TileType::stairs;
      }

      return TileType::undefined;
    }
  };
}
