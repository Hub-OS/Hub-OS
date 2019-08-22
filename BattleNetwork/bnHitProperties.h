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