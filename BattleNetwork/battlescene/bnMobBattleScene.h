#pragma once
#include "bnBattleSceneBase.h"

class Player;
class Mob;
struct RetreatBattleState;
struct CombatBattleState;
struct TimeFreezeBattleState;
struct MobIntroBattleState;
class CharacterTransformBattleState;
class CardSelectBattleState;

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
  sf::Sprite mug; // speaker mugshot
  Animation anim; // mugshot animation
  std::shared_ptr<sf::Texture> emotion; // emotion atlas image
  std::vector<std::string> blocks;
};

/*
    \brief Battle scene configuration for a regular PVE battle
*/
class MobBattleScene final : public BattleSceneBase {
  MobBattleProperties props;
  bool playerDecross{ false };
  int playerHitCount{};
  TimeFreezeBattleState* timeFreezePtr{ nullptr };
  CombatBattleState* combatPtr{ nullptr };

  // Battle state hooks
  std::function<bool()> HookIntro(MobIntroBattleState& intro, TimeFreezeBattleState& timefreeze, CombatBattleState& combat);
  std::function<bool()> HookRetreat(RetreatBattleState& retreat, FadeOutBattleState& fadeout);
  std::function<bool()> HookFormChangeEnd(CharacterTransformBattleState& form, CardSelectBattleState& cardSelect);
  std::function<bool()> HookFormChangeStart(CharacterTransformBattleState& form);
  std::function<bool()> HookPlayerWon();
  public:
  MobBattleScene(swoosh::ActivityController& controller, MobBattleProperties props, BattleResultsFunc onEnd=nullptr);
  ~MobBattleScene();

  void Init() override;

  void OnHit(Entity& victim, const Hit::Properties& props) override final;
  void onUpdate(double elapsed) override final;
  void onStart() override final;
  void onExit() override;
  void onEnter() override;
  void onResume() override;
  void onLeave() override;
};