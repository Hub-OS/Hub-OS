/*! \brief The possible states of Tiles
 */

#pragma once

enum class TileState : int {
  normal = 0,
  cracked,
  broken,
  ice,
  grass,
  lava,
  poison,
  empty,
  holy,
  directionLeft,
  directionRight,
  directionUp,
  directionDown,
  volcano,
  sea,
  sand,
  metal,
  hidden, // immutable
  size // no a valid state! used for enum length
};