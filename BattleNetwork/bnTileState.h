/*! \brief The possible states of Tiles
 */

#pragma once
<<<<<<< HEAD
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
=======
enum class TileState : const int {
  NORMAL = 0,
  CRACKED,
  BROKEN,
  ICE,
  GRASS,
  LAVA,
  POISON,
  EMPTY
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
};