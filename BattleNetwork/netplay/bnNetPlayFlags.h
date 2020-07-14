#pragma once
#include <string>
#include "../bnDirection.h"

struct NetPlayFlags {
  bool isRemotePlayerLoser{ false };
  bool isRemoteConnected{ false };
  bool isRemoteReady{ false };
  bool remoteShoot{ false };
  bool remoteUseSpecial{ false };
  bool remoteCharge{ false };
  bool remoteIsFormChanging{ false };
  bool remoteIsLeavingFormChange{ false };
  bool remoteIsAnimatingFormChange{ false };
  int remoteHP{ 1 };
  int remoteTileX{ 5 }, remoteTileY{ 2 };
  int remoteFormSelect{ 0 };
  int remoteLastFormSelect{ 0 };
  Direction remoteDirection{ Direction::none };
  int remoteNavi{ 0 };
  std::string remoteChipUse{ "" };
};