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
    ping = 0,      // 0
    ack,           // 1 acknowledge packet
    login,         // 2 login request
    logout,        // 3 logout notification
    loaded_map,    // 4 avatar loaded map
    user_xyz,      // 5 reporting avatar world location
    avatar_change, // 6 avatar was switched
    emote,         // 7 player emoted
    size,
    unknown = size
  };

  enum class ServerEvents : uint16_t
  {
    pong = 0,      // 0
    ack,           // 1
    login,         // 2
    map,           // 3
    avatar_join,   // 4
    logout,        // 5
    hologram_name, // 6
    hologram_xyz,  // 7
    avatar_change, // 8
    emote,         // 9
    size,
    unknown = size
  };
} // namespace Overworld
