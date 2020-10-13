#pragma once
#include "bnBattleSceneBase.h"

class Player; //!< Forward declare

// PVP properties the scene needs
struct PVPBattleProperties {
  BattleSceneBaseProps base;
  int rounds;
};

class PVPBattleScene final : public BattleSceneBase {
  Player* remotePlayer; // the other person

  PVPBattleScene(const PVPBattleProperties& props);

  ~PVPBattleScene();
};
