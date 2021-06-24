#pragma once
#include <string_view>
#include "../stx/string.h"

namespace Overworld {
  namespace ObjectType {
    enum ObjectType {
      custom_warp,
      server_warp,
      position_warp,
      custom_server_warp,
      home_warp,
      board,
      shop,
      undefined,
    };

    inline ObjectType FromString(const std::string_view& type_string) {
      constexpr std::string_view custom_warp_string = "Custom Warp";
      constexpr std::string_view server_warp_string = "Server Warp";
      constexpr std::string_view position_warp_string = "Position Warp";
      constexpr std::string_view custom_server_warp_string = "Custom Server Warp";
      constexpr std::string_view home_warp_string = "Home Warp";
      constexpr std::string_view board_string = "Board";
      constexpr std::string_view shop_string = "Shop";

      if (stx::insensitive_equals(type_string, custom_warp_string)) {
        return ObjectType::custom_warp;
      }
      if (stx::insensitive_equals(type_string, server_warp_string)) {
        return ObjectType::server_warp;
      }
      if (stx::insensitive_equals(type_string, position_warp_string)) {
        return ObjectType::position_warp;
      }
      if (stx::insensitive_equals(type_string, custom_server_warp_string)) {
        return ObjectType::custom_server_warp;
      }
      if (stx::insensitive_equals(type_string, home_warp_string)) {
        return ObjectType::home_warp;
      }
      if (stx::insensitive_equals(type_string, board_string)) {
        return ObjectType::board;
      }
      if (stx::insensitive_equals(type_string, shop_string)) {
        return ObjectType::shop;
      }

      return ObjectType::undefined;
    }

    inline bool IsWarp(ObjectType type) {
      return type == custom_warp ||
        type == server_warp ||
        type == position_warp ||
        type == custom_server_warp ||
        type == home_warp;
    }

    inline bool IsCustomWarp(ObjectType type) {
      return type == custom_warp || type == custom_server_warp;
    }
  };
}
