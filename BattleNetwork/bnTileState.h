/*! \brief The possible states of Tiles
 *  \class TileState
 */

#pragma once
enum class TileState : const int {
  NORMAL,
  CRACKED,
  BROKEN,
  ICE,
  GRASS,
  LAVA,
  POISON,
  EMPTY
};