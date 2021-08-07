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
  trade_card_list, // Swap list data with opponent
  card_list_request, // Ask to download only specific cards
  card_list_download, // Download and deserialize card data
  player_package_download, // Download player package data
  downloads_complete,

  ///////////////////////
  //     Misc. Cmds    //
  ///////////////////////
  ping // to keep from kicking 
};