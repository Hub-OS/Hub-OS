#pragma once
#include <string_view>
#include "../stx/string.h"

namespace Overworld {
  namespace TileType {
    enum TileType {
      conveyor,
      stairs,
      ice,
      undefined,
    };

    inline TileType FromString(const std::string_view& type_string) {
      constexpr std::string_view conveyor_string = "Conveyor";
      constexpr std::string_view stairs_string = "Stairs";
      constexpr std::string_view ice_string = "Ice";

      if (stx::insensitive_equals(type_string, conveyor_string)) {
        return TileType::conveyor;
      }
      if (stx::insensitive_equals(type_string, stairs_string)) {
        return TileType::stairs;
      }
      if (stx::insensitive_equals(type_string, ice_string)) {
        return TileType::ice;
      }

      return TileType::undefined;
    }
  };
}
