#pragma once
#include <string_view>
#include "../stx/string.h"

namespace Overworld {
  namespace TileType {
    enum TileType {
      conveyor,
      custom_warp,
      server_warp,
      position_warp,
      custom_server_warp,
      home_warp,
      board,
      shop,
      stairs,
      undefined,
    };

    inline TileType FromString(const std::string_view& type_string) {
      constexpr std::string_view conveyor_string = "Conveyor";
      constexpr std::string_view custom_warp_string = "Custom Warp";
      constexpr std::string_view server_warp_string = "Server Warp";
      constexpr std::string_view position_warp_string = "Position Warp";
      constexpr std::string_view custom_server_warp_string = "Custom Server Warp";
      constexpr std::string_view home_warp_string = "Home Warp";
      constexpr std::string_view board_string = "Board";
      constexpr std::string_view shop_string = "Shop";
      constexpr std::string_view stairs_string = "Stairs";

      if (stx::insensitive_equals(type_string, conveyor_string)) {
        return TileType::conveyor;
      }
      if (stx::insensitive_equals(type_string, custom_warp_string)) {
        return TileType::custom_warp;
      }
      if (stx::insensitive_equals(type_string, server_warp_string)) {
        return TileType::server_warp;
      }
      if (stx::insensitive_equals(type_string, position_warp_string)) {
        return TileType::position_warp;
      }
      if (stx::insensitive_equals(type_string, custom_server_warp_string)) {
        return TileType::custom_server_warp;
      }
      if (stx::insensitive_equals(type_string, home_warp_string)) {
        return TileType::home_warp;
      }
      if (stx::insensitive_equals(type_string, board_string)) {
        return TileType::board;
      }
      if (stx::insensitive_equals(type_string, shop_string)) {
        return TileType::shop;
      }
      if (stx::insensitive_equals(type_string, stairs_string)) {
        return TileType::stairs;
      }

      return TileType::undefined;
    }

    inline bool IsWarp(TileType type) {
      return type == custom_warp ||
        type == server_warp ||
        type == position_warp ||
        type == custom_server_warp ||
        type == home_warp;
    }

    inline bool IsCustomWarp(TileType type) {
      return type == custom_warp || type == custom_server_warp;
    }
  };
}
