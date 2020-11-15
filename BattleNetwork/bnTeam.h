/*! \brief Entities can be on team RED, BLUE, or UNKNOWN for agnostic types */

#pragma once
enum class Team : int {
  unknown = 0,
  blue,
  red
};