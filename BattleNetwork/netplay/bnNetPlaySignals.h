#pragma once

enum class NetPlaySignals : unsigned int {
  none = 0,
  connect,
  handshake,
  ready,
  form,
  move,
  hp,
  tile,
  chip,
  loser,
  shoot,
  special,
  charge,
  card_select,
};