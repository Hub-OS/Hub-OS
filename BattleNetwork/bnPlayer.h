#pragma once

#include "bnCharacter.h"
#include "bnPlayerState.h"
#include "bnTextureType.h"
#include "bnPlayerHealthUI.h"
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
  virtual const bool Hit(int _damage, HitProperties props = Entity::DefaultHitProperties);
  int GetMoveCount() const;
  int GetHitCount() const;

  PlayerHealthUI* GetHealthUI() const;
  AnimationComponent& GetAnimationComponent();

  void SetCharging(bool state);
  void SetAlpha(int value); // TODO: Get rid of this
  void SetCloakTimer(int seconds); // TODO: Get rid
  virtual void SetAnimation(string _state, std::function<void()> onFinish = nullptr);

protected:

  // TODO: get rid of this
  sf::Clock cloakTimer;
  int cloakTimeSecs;

  int health;
  int hitCount;

  int alpha;
  double invincibilityCooldown;

  TextureType textureType;
  string state;
  PlayerHealthUI* healthUI;

  //-Animation-
  float animationProgress;

  ChargeComponent chargeComponent;
  AnimationComponent animationComponent;
};