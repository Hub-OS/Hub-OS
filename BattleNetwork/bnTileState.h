/*! \brief The possible states of Tiles
 */

#pragma once

enum class TileState : int {
  NORMAL = 0,
  CRACKED = 1,
  BROKEN = 2,
  ICE = 3,
  GRASS = 4,
  LAVA = 5,
  POISON = 6,
  EMPTY = 7,
  HOLY = 9,
  DIRECTION_LEFT = 10,
  DIRECTION_RIGHT = 11,
  DIRECTION_UP = 12,
  DIRECTION_DOWN = 13,
  VOLCANO = 14,
  SIZE = 8
};