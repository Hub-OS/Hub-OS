/*! \brief The possible states of Tiles
 */

#pragma once

enum class TileState : int {
  normal = 0,
  cracked = 1,
  broken = 2,
  ice = 3,
  grass = 4,
  lava = 5,
  poison = 6,
  empty = 7,
  holy = 9,
  directionLeft = 10,
  directionRight = 11,
  directionUp = 12,
  directionDown = 13,
  volcano = 14,
  size = 15
};