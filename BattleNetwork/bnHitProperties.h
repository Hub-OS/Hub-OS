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
    Character* aggressor;
    Direction drag; // Used by dragging payload

    static Properties GetDefaultProperties() {
      return Properties(0, Flags(Hit::recoil | Hit::impact), Element::NONE, nullptr, Direction::NONE);
    }


    Properties(int damage = 0, Flags flags = 0x00, Element element = Element::NONE, Character* aggressor = nullptr, Direction drag = Direction::NONE) 
      : damage(damage), flags(flags), element(element), aggressor(aggressor), drag(drag) { }
    
    Properties(const Properties& rhs) {
      damage = rhs.damage;
      flags = rhs.flags;
      element = rhs.element;
      aggressor = rhs.aggressor;
      drag = rhs.drag;
    }

    ~Properties() = default;
  };


  const Properties DefaultProperties = Properties::GetDefaultProperties();

}