#pragma once
#include "bnBattleSceneBase.h"
#include "States/bnCharacterTransformBattleState.h" // defines TrackedFormData struct

class Player;
class Mob;
struct FreedomMissionOverState;

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
};

/*
    \brief Battle scene configuration for a regular PVE battle
*/
class FreedomMissionMobScene final : public BattleSceneBase {
  FreedomMissionOverState* overStatePtr{ nullptr };
  FreedomMissionProps props;
  std::vector<Player*> players;
  std::vector<std::shared_ptr<TrackedFormData>> trackedForms;
  bool playerDecross{ false };
  int playerHitCount{};

  public:
  FreedomMissionMobScene(swoosh::ActivityController& controller, const FreedomMissionProps& props, BattleResultsFunc onEnd=nullptr);
  ~FreedomMissionMobScene();

  void OnHit(Character& victim, const Hit::Properties& props) override final;
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