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

  DefenseRule* virusBody;
  AnimationComponent* animationComponent;
  SpriteSceneNode *acid, *head, *side, *leftShoulder, *rightShoulder, *leftShoulderShoot, *rightShoulderShoot;
  Animation animation;
  float totalElapsed, coreRegen;
  float hitHeight;
  int coreHP, prevCoreHP;

  AlphaArm* leftArm;
  AlphaArm* rightArm;

  bool firstTime;
  bool impervious;
  bool shootSuperVulcans;

  class AlphaCoreDefenseRule : public DefenseRule {
    int& alphaCoreHP;

  public:
    AlphaCoreDefenseRule(int& alphaCoreHP);
    virtual ~AlphaCoreDefenseRule();
    virtual const bool Blocks(Spell* in, Character* owner);
    virtual Hit::Properties& FilterStatuses(Hit::Properties& statuses);
  } *defense;
public:
  using DefaultState = AlphaIdleState;

  AlphaCore(Rank _rank = Character::Rank::_1);

  ~AlphaCore();

  /**
   * @brief When health is low, deletes. Updates AI
   * @param _elapsed
   */
  void OnUpdate(float _elapsed);

  const float GetHeight() const;

  const bool OnHit(const Hit::Properties props);
  void OnDelete();

  void OpenShoulderGuns();
  void CloseShoulderGuns();
  void HideLeftArm();
  void RevealLeftArm();
  void HideRightArm();
  void RevealRightArm();
  void EnableImpervious(bool impervious = true);
  void ShootSuperVulcans();
};