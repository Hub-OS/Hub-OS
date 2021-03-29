#pragma once

#include <stdint.h>
#include <string>

namespace Overworld
{
  const std::string VERSION_ID = "https://github.com/ArthurCose/Scriptable-OpenNetBattle-Server/tree/development";
  const uint64_t VERSION_ITERATION = 7;

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
    asset_stream,
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

  enum ClientAssetType : char {
    texture,
    animation,
  };

  enum class ServerEvents : uint16_t
  {
    pong = 0,
    ack,
    login,
    kick,
    remove_asset,
    asset_stream,
    asset_stream_complete,
    map,
    exclude_object,
    include_object,
    transfer_start,
    transfer_complete,
    move_camera,
    slide_camera,
    unlock_camera,
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
    navi_set_direction,
    navi_set_avatar,
    navi_emote,
    navi_animate,
    size,
    unknown = size
  };

  enum class AssetType : char {
    text = 0,
    texture,
    audio
  };
} // namespace Overworld
