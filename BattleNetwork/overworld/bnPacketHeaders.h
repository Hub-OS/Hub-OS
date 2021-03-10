#pragma once

#include <stdint.h>
#include <string>

namespace Overworld
{
  const std::string VERSION_ID = "https://github.com/ArthurCose/Scriptable-OpenNetBattle-Server/tree/proposed-packets";
  const uint64_t VERSION_ITERATION = 12;

  constexpr double PACKET_RESEND_RATE = 1.0 / 20.0;

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
    ping = 0,
    ack,
    asset_found,
    texture_stream,
    animation_stream,
    login,
    logout,
    request_join,
    ready,
    position,
    avatar_change,
    emote,
    object_interaction,
    navi_interaction,
    tile_interaction,
    dialog_response,
    size,
    unknown = size
  };

  enum class ServerEvents : uint16_t
  {
    pong = 0,
    ack,
    login,
    remove_asset,
    asset_stream,
    asset_stream_complete,
    map,
    transfer_start,
    transfer_complete,
    lock_input,
    unlock_input,
    move,
    message,
    question,
    quiz,
    navi_connected,
    navi_disconnect,
    navi_set_name,
    navi_move_to,
    navi_set_avatar,
    navi_emote,
    size,
    unknown = size
  };

  enum class AssetType : char {
    text = 0,
    texture,
    audio,
    sfml_image
  };
} // namespace Overworld
