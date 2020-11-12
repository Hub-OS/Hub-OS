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
  int clientDamage{};
  int remoteDamage{};

public:
  PVPBattleScene(swoosh::ActivityController& controller, const PVPBattleProperties& props);
  ~PVPBattleScene();
  void OnHit(Character& victim, const Hit::Properties& props) override final;
};
