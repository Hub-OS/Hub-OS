#pragma once
#include <SFML/Graphics.hpp>
using sf::IntRect;

#include "bnCharacter.h"
#include "bnMobState.h"
#include "bnTextureType.h"
#include "bnMobHealthUI.h"
#include "bnProgsManIdleState.h"
#include "bnAnimationComponent.h"
#include "bnAI.h"

class ProgsMan : public Character, public AI<ProgsMan> {
public:
  friend class ProgsManIdleState;
  friend class ProgsManMoveState;
  friend class ProgsManAttackState;

  ProgsMan(Rank _rank);
  virtual ~ProgsMan(void);

  virtual void Update(float _elapsed);
  virtual void RefreshTexture();
  virtual vector<Drawable*> GetMiscComponents();
  virtual void SetAnimation(string _state, std::function<void()> onFinish = nullptr);
  virtual void SetCounterFrame(int frame);
  virtual void OnFrameCallback(int frame, std::function<void()> onEnter, std::function<void()> onLeave = nullptr, bool doOnce = false);
  virtual int GetHealth() const;
  virtual TextureType GetTextureType() const;

  void SetHealth(int _health);
  virtual const bool Hit(int _damage, Hit::Properties props = Entity::DefaultHitProperties);

  virtual const float GetHitHeight() const;
  virtual int* GetAnimOffset();

private:
  sf::Clock clock;
 
  AnimationComponent animationComponent;

  float hitHeight;
  string state;
  TextureType textureType;
  MobHealthUI* healthUI;
  sf::Shader* whiteout;
  sf::Shader* stun;

};