#pragma once
#include <string_view>
#include "../stx/string.h"

namespace Overworld {
  namespace TileShadow {
    enum TileShadow {
      automatic,
      always,
      never
    };

    inline TileShadow FromString(const std::string_view& type_string) {
      constexpr std::string_view always_string = "Always";
      constexpr std::string_view never_string = "Never";

      if (stx::insensitive_equals(type_string, always_string)) {
        return TileShadow::always;
      }
      if (stx::insensitive_equals(type_string, never_string)) {
        return TileShadow::never;
      }

      return TileShadow::automatic;
    }
  };
}
