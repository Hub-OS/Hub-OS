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
  connect,
  handshake, // send round information along with hand and form selections
  form, // used when we are de-formed from battle
  hp, // emit our current health
  tile, // emit our tile location 
  card, // emit our card data
  loser,
  input_event,
  card_select, // used when player opens card select widget,

  ///////////////////////
  // PVP Download Cmds //
  ///////////////////////
  download_handshake,
  trade_web_card_list, // Swap list data with opponent
  web_card_list_request, // Ask to download only specific cards
  web_card_list_download, // Download and deserialize card data
  trade_player_package, // Swap player package data with opponent
  trade_card_package_list, // Swap card packages list with opponent
  player_package_request, // Ask to download only specific package
  player_package_download, // Download player package data
  card_package_request, // Ask to download only specific package
  card_package_download, // Download card package data
  downloads_complete,

  ///////////////////////
  //     Misc. Cmds    //
  ///////////////////////
  ping // to keep from kicking 
};