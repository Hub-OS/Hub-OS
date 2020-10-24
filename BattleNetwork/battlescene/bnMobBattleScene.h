#pragma once
#include "bnBattleSceneBase.h"

class Player;
class Mob;

/*
    \brief Lots of properties packed into a clean struct
*/
struct MobBattleProperties {
  BattleSceneBaseProps base;
  enum class RewardBehavior : int {
      take = 0,    //!< One reward at the end of all mobs
      takeForEach, //!< Each mobs rewards the player
      none         //!< No rewards given
  } reward{ };
  std::vector<Mob*> mobs;
};

/*
    \brief Battle scene configuration for a regular PVE battle
*/
class MobBattleScene final : public BattleSceneBase {
  MobBattleProperties props;

  public:
  MobBattleScene(swoosh::ActivityController& controller, const MobBattleProperties& props);
  ~MobBattleScene();

  void onStart() override final;
  void onExit() override;
  void onEnter() override;
  void onResume() override;
  void onLeave() override;
  void onEnd() override;
};