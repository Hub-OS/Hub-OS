#pragma once

enum class NetPlaySignals : unsigned int {
  none = 0,
  ///////////////////////
  //       ACKs        //
  ///////////////////////
  ack,

  ///////////////////////
  //    Matchmaking    //
  ///////////////////////
  matchmaking_request,
  matchmaking_handshake,

  ///////////////////////
  //  PVP Battle Cmds  // 
  ///////////////////////
  handshake, // send round information along with hand and form selections
  form, // used when we are de-formed from battle
  frame_data,

  ///////////////////////
  // PVP Download Cmds //
  ///////////////////////
  download_handshake,
  trade_player_package, // Swap player package data with opponent
  trade_card_package_list, // Swap card packages list with opponent
  trade_block_package_list, // Swap block packages list with opponent
  player_package_request, // Ask to download only specific package
  player_package_download, // Download player package data
  card_package_request, // Ask to download only specific package
  card_package_download, // Download card package data
  block_package_request, // Ask to download only specific package
  block_package_download, // Download block package data
  downloads_complete,

  ///////////////////////
  //     Misc. Cmds    //
  ///////////////////////
  ping // to keep from kicking 
};