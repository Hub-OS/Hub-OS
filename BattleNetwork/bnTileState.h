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
  SIZE = 8
};