#pragma once

#include <stdint.h>
#include <string>

namespace Overworld
{
  const std::string VERSION_ID = "https://github.com/ArthurCose/Scriptable-OpenNetBattle-Server/tree/proposed-packets";
  const uint64_t VERSION_ITERATION = 0;

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
    ready,         // 4 avatar loaded map
    position,      // 5 reporting avatar world location
    avatar_change, // 6 avatar was switched
    emote,         // 7 player emoted
    size,
    unknown = size
  };

  enum class ServerEvents : uint16_t
  {
    pong = 0,        // 0
    ack,             // 1
    login,           // 2
    map,             // 3
    navi_connected,  // 4
    navi_disconnect, // 5
    navi_set_name,   // 6
    navi_move_to,    // 7
    navi_set_avatar, // 8
    navi_emote,      // 9
    size,
    unknown = size
  };
} // namespace Overworld
