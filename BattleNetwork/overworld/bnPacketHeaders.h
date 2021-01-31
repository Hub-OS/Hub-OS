#pragma once

#include <stdint.h>

namespace Overworld
{
  enum class Reliability : char
  {
    Unreliable = 0,
    UnreliableSequenced,
    Reliable,
    ReliableSequenced,
    ReliableOrdered,
  };

  // server expects uint16_t codes
  enum class ClientEvents : uint16_t
  {
    login = 0,     // 0 login request
    user_xyz,      // 1 reporting avatar world location
    logout,        // 2 logout notification
    loaded_map,    // 3 avatar loaded map
    avatar_change, // 4 avatar was switched
    emote,         // 5 player emoted
    ping,          // 6
    ack,           // 7 acknowledge packet
    size,
    unknown = size
  };

  enum class ServerEvents : uint16_t
  {
    login = 0,     // 0
    hologram_xyz,  // 1
    hologram_name, // 2
    time_of_day,   // 3
    map,           // 4
    logout,        // 5
    avatar_change, // 6
    emote,         // 7
    avatar_join,   // 8
    pong,          // 9
    ack,           // 10
    size,
    unknown = size
  };
} // namespace Overworld
