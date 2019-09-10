#pragma once
#include "bnAI.h"
#include "bnCharacter.h"
#include "bnAlphaIdleState.h"
#include "bnAnimation.h"
#include "bnDefenseRule.h"

class AnimationComponent;

class AlphaCore : public Character, public AI<AlphaCore> {
  friend class AlphaIdleState;

  DefenseRule* virusBody;
  AnimationComponent* animationComponent;
  SpriteSceneNode *acid, *head, *side;
  Animation animation;
  float totalElapsed, coreRegen;
  float hitHeight;
  int coreHP, prevCoreHP;

  class AlphaCoreDefenseRule : public DefenseRule {
    int& alphaCoreHP;

  public:
    AlphaCoreDefenseRule(int& alphaCoreHP);
    virtual ~AlphaCoreDefenseRule();
    virtual const bool Check(Spell* in, Character* owner);
  } *defense;

public:
  using DefaultState = AlphaIdleState;

  AlphaCore(Rank _rank = Character::Rank::_1);

  virtual ~AlphaCore();

  /**
   * @brief When health is low, deletes. Updates AI
   * @param _elapsed
   */
  virtual void OnUpdate(float _elapsed);

  virtual const float GetHitHeight() const;

  virtual const bool OnHit(const Hit::Properties props);
  virtual void OnDelete();
};