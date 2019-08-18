#pragma once
#include "bnElements.h"
#include "bnDirection.h"

class Character;

namespace Hit {
  typedef unsigned char Flags;

  const Flags none = 0x00;
  const Flags recoil = 0x01;
  const Flags shake = 0x02;
  const Flags stun = 0x04;
  const Flags pierce = 0x08;
  const Flags flinch = 0x10;
  const Flags breaking = 0x20;
  const Flags impact = 0x40;
  const Flags drag = 0x80;

  /**
   * @struct Properties
   * @author mav
   * @date 05/05/19
   * @brief Hit box information
   */
  struct Properties {
    int damage;
    Flags flags;
    Element element;
    double secs; // used by both recoil and stun
    Character* aggressor;
    Direction drag; // Used by dragging payload

    Properties() = default;
    
    Properties(const Properties& rhs) = default;

    ~Properties() = default;
  };

  const Properties DefaultProperties{ 0, Flags(Hit::recoil | Hit::impact), Element::NONE, 3.0, nullptr, Direction::NONE };

}