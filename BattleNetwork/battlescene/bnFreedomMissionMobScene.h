#pragma once
#include "bnBattleSceneBase.h"

class Player;
class Mob;

struct CombatBattleState;
struct FreedomMissionOverState;
struct TimeFreezeBattleState;

/*
    \brief Lots of properties packed into a clean struct
*/
struct FreedomMissionProps {
  BattleSceneBaseProps base;
  std::vector<Mob*> mobs;
  uint8_t maxTurns{ 3 };
  sf::Sprite mug; // speaker mugshot
  Animation anim; // mugshot animation
  std::shared_ptr<sf::Texture> emotion; // emotion atlas image
  std::vector<std::string> blocks;
};

/*
    \brief Battle scene configuration for a regular PVE battle
*/
class FreedomMissionMobScene final : public BattleSceneBase {
  FreedomMissionOverState* overStatePtr{ nullptr };
  FreedomMissionProps props;
  bool playerDecross{ false };
  int playerHitCount{};
  TimeFreezeBattleState* timeFreezePtr{ nullptr };
  CombatBattleState* combatPtr{ nullptr };

  public:
  FreedomMissionMobScene(swoosh::ActivityController& controller, FreedomMissionProps props, BattleResultsFunc onEnd=nullptr);
  ~FreedomMissionMobScene();

  void Init() override;

  void OnHit(Entity& victim, const Hit::Properties& props) override final;
  void onUpdate(double elapsed) override final;
  void onStart() override final;
  void onExit() override;
  void onEnter() override;
  void onResume() override;
  void onLeave() override;

  // 
  // override
  //

  void IncrementTurnCount() override;
};