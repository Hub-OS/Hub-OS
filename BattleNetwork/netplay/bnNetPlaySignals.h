#pragma once

enum class NetPlaySignals : unsigned int {
  none = 0,
  ack,
  connect,
  handshake, // send round information along with hand and form selections
  form, // used when we are de-formed from battle
  hp, // emit our current health
  tile, // emit our tile location 
  card, // emit our card data
  loser,
  shoot,
  special,
  charge,
  card_select // used when player opens card select widget
};