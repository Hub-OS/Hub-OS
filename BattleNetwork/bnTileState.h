/*! \brief The possible states of Tiles
 */

#pragma once
enum class TileState : const int {
  NORMAL = 0,
  CRACKED,
  BROKEN,
  ICE,
  GRASS,
  LAVA,
  POISON,
  EMPTY
};