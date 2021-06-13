#pragma once
#include "bnBossPatternAI.h"
#include "bnCharacter.h"
#include "bnAlphaIdleState.h"
#include "bnAnimation.h"
#include "bnDefenseRule.h"

class AnimationComponent;
class Obstacle;
class AlphaArm;

class AlphaCore : public Character, public BossPatternAI<AlphaCore> {
  friend class AlphaIdleState;

  DefenseRule* virusBody{ nullptr };
  AnimationComponent* animationComponent{ nullptr };
  SpriteProxyNode* acid{ nullptr }, *head{ nullptr }, *side{ nullptr }, *leftShoulder{ nullptr }, 
    *rightShoulder{ nullptr }, *leftShoulderShoot{ nullptr }, *rightShoulderShoot{ nullptr };
  Animation animation;
  double totalElapsed, coreRegen;
  float hitHeight;
  int coreHP, prevCoreHP;

  AlphaArm* leftArm{ nullptr };
  AlphaArm* rightArm{ nullptr };

  bool impervious;
  bool shootSuperVulcans;

  /**
  * @brief a defense rule specifically for alpha
  */
  class AlphaCoreDefenseRule : public DefenseRule {
    int& alphaCoreHP;

  public:
    AlphaCoreDefenseRule(int& alphaCoreHP);
    ~AlphaCoreDefenseRule();
    void CanBlock(DefenseFrameStateJudge& judge, Spell& in, Character& owner) override;
    Hit::Properties& FilterStatuses(Hit::Properties& statuses) override;
  } *defense{ nullptr };
public:
  using DefaultState = AlphaIdleState;

  AlphaCore(Rank _rank = Character::Rank::_1);

  ~AlphaCore();

  /**
   * @brief When health is low, deletes. Updates AI
   * @param _elapsed
   */
  void OnUpdate(double _elapsed) override;
  const float GetHeight() const override;
  void OnSpawn(Battle::Tile& start) override;

  void OnDelete() override;
  bool CanMoveTo(Battle::Tile* next) override;

  void OpenShoulderGuns();
  void CloseShoulderGuns();
  void HideLeftArm();
  void RevealLeftArm();
  void HideRightArm();
  void RevealRightArm();
  void EnableImpervious(bool impervious = true);
  void ShootSuperVulcans();
};