#pragma once
#include <string>
#include "../bnDirection.h"

struct NetPlayFlags {
  bool openedCardWidget{ false };
  bool isRemoteConnected{ false };
  bool remoteShoot{ false };
  bool remoteUseSpecial{ false };
  bool remoteCharge{ false };
  int remoteTileX{ 5 }, remoteTileY{ 2 };
  int remoteFormSelect{ -1 };
  int remoteNavi{ 0 };
  int remoteHP{ 1 };
  std::string remoteChipUse{ "" };
};