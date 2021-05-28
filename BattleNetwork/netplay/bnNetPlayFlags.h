#pragma once
#include <string>
#include <vector>
#include "../bnInputEvent.h"
#include "../bnDirection.h"

struct NetPlayFlags {
  bool openedCardWidget{ false };
  bool remoteHandshake{ false };
  bool remoteChangeForm{ false };
  bool remoteConnected{ false };
  int remoteTileX{ 5 }, remoteTileY{ 2 };
  int remoteFormSelect{ -1 };
  int remoteNavi{ 0 };
  int remoteHP{ 1 };
  std::vector<InputEvent> remoteInputEvents;
};