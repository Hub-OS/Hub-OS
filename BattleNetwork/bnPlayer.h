#pragma once

#include "bnCharacter.h"
#include "bnPlayerState.h"
#include "bnTextureType.h"
#include "bnChargeComponent.h"
#include "bnAnimationComponent.h"
#include "bnAI.h"
#include "bnPlayerControlledState.h"
#include "bnPlayerIdleState.h"
#include "bnPlayerHitState.h"

using sf::IntRect;

class Player : public Character, public AI<Player> {
public:
  friend class PlayerControlledState;
  friend class PlayerIdleState;
  friend class PlayerHitState;

  Player(void);
  virtual ~Player(void);

  virtual void Update(float _elapsed);
  void Attack(float _charge);
  virtual vector<Drawable*> GetMiscComponents();

  virtual int GetHealth() const;
  virtual void SetHealth(int _health);
  virtual const bool Hit(int _damage, Hit::Properties props = Hit::DefaultProperties);
  int GetMoveCount() const;
  int GetHitCount() const;

  AnimationComponent& GetAnimationComponent();

  void SetCharging(bool state);

  virtual void SetAnimation(string _state, std::function<void()> onFinish = nullptr);

protected:
  int health;
  int hitCount;

  double invincibilityCooldown;

  TextureType textureType;
  string state;

  //-Animation-
  float animationProgress;

  ChargeComponent chargeComponent;
  AnimationComponent animationComponent;
};