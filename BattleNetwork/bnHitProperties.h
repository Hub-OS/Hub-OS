#pragma once
#include "bnElements.h"
#include "bnDirection.h"

class Character;

namespace Hit {
  using Flags = uint16_t;

  // These are in order!
  // Hitboxes properties will be inserted in queue
  // based on priority (A < B) where the highest priority
  // is the lowest value
  const Flags none = 0x0000;
  const Flags retangible = 0x0001;
  const Flags freeze = 0x0002;
  const Flags pierce = 0x0004;
  const Flags recoil = 0x0008;
  const Flags shake = 0x0010;
  const Flags stun = 0x0020;
  const Flags flinch = 0x0040;
  const Flags breaking = 0x0080;
  const Flags impact = 0x0100;
  const Flags drag = 0x0200;
  const Flags bubble = 0x0400;
  // 0x0800 - 0x8000 remaining = 5 remaining


  struct Drag {
    Direction dir{ Direction::none };
    unsigned count{ 0 };
  };

  /**
   * @struct Properties
   * @author mav
   * @date 05/05/19
   * @brief Hit box information
   */
  struct Properties {
    int damage{};
    Flags flags{ none };
    Element element{ Element::none };
    Character* aggressor{ nullptr };
    Drag drag{ }; // Used by Hit::drag flag
    bool counters{ true };

    Properties() = default;
    Properties(const Properties& rhs) = default;
    ~Properties() = default;
  };

  const constexpr Hit::Properties DefaultProperties = { 
    0, 
    Flags(Hit::recoil | Hit::impact), 
    Element::none, 
    nullptr, 
    Direction::none,
    true
  };
}