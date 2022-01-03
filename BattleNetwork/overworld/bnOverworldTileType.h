#pragma once
#include <string_view>
#include "../stx/string.h"

namespace Overworld {
  namespace TileType {
    enum TileType {
      conveyor,
      stairs,
      ice,
      treadmill,
      invisible,
      arrow,
      undefined,
    };

    inline TileType FromString(const std::string_view& type_string) {
      constexpr std::string_view conveyor_string = "Conveyor";
      constexpr std::string_view stairs_string = "Stairs";
      constexpr std::string_view ice_string = "Ice";
      constexpr std::string_view treadmill_string = "Treadmill";
      constexpr std::string_view invisible_string = "Invisible";
      constexpr std::string_view arrow_string = "Arrow";

      if (stx::insensitive_equals(type_string, conveyor_string)) {
        return TileType::conveyor;
      }
      if (stx::insensitive_equals(type_string, stairs_string)) {
        return TileType::stairs;
      }
      if (stx::insensitive_equals(type_string, ice_string)) {
        return TileType::ice;
      }
      if (stx::insensitive_equals(type_string, treadmill_string)) {
        return TileType::treadmill;
      }
      if (stx::insensitive_equals(type_string, invisible_string)) {
        return TileType::invisible;
      }
      if (stx::insensitive_equals(type_string, arrow_string)) {
        return TileType::arrow;
      }

      return TileType::undefined;
    }
  };
}
