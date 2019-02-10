#pragma once
#include <SFML/Graphics.hpp>
using sf::IntRect;

#include "bnCharacter.h"
#include "bnMobState.h"
#include "bnTextureType.h"
#include "bnMobHealthUI.h"
#include "bnMetalManIdleState.h"
#include "bnAnimationComponent.h"
#include "bnAI.h"

class MetalMan : public Character, public AI<MetalMan> {
public:
  friend class MetalManIdleState;
  friend class MetalManMoveState;
  friend class MetalManPunchState;

  MetalMan(Rank _rank);
  virtual ~MetalMan();

  virtual void Update(float _elapsed);
  virtual void RefreshTexture();
  virtual bool CanMoveTo(Battle::Tile * next);
  virtual vector<Drawable*> GetMiscComponents();
  virtual void SetAnimation(string _state, std::function<void()> onFinish = nullptr);
  virtual void SetCounterFrame(int frame);
  virtual void OnFrameCallback(int frame, std::function<void()> onEnter, std::function<void()> onLeave = nullptr, bool doOnce = false);
  virtual int GetHealth() const;
  virtual TextureType GetTextureType() const;

  void SetHealth(int _health);
  virtual const bool Hit(int _damage, Hit::Properties props = Character::DefaultHitProperties);

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

  bool movedByStun;

};