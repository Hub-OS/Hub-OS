/*! \brief Entities can be on team RED, BLUE, or UNKNOWN for agnostic types */

#pragma once
enum class Team : int {
  unset = -1,
  unknown = 0,
  blue,
  red
};