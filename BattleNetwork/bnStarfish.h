#pragma once
#include "bnAnimatedCharacter.h"
#include "bnMobState.h"
#include "bnAI.h"
#include "bnTextureType.h"
#include "bnMobHealthUI.h"

class Starfish : public AnimatedCharacter, public AI<Starfish> {
  friend class StarfishIdleState;
  friend class StarfishAttackState;

public:
  Starfish(Rank _rank = Rank::_1);
  virtual ~Starfish(void);

  virtual void Update(float _elapsed);
  virtual void RefreshTexture();
  //virtual void SetAnimation(string _state, std::function<void()> onFinish = nullptr);
  //virtual void SetCounterFrame(int frame);
  virtual int GetHealth() const;

  void SetHealth(int _health);
  virtual int* GetAnimOffset();
  virtual const bool Hit( Hit::Properties props = Hit::DefaultProperties);
  virtual const float GetHitHeight() const;

private:
  sf::Shader* whiteout;
  sf::Shader* stun;

  string state;

  float hitHeight;
  bool hit;
  TextureType textureType;
  MobHealthUI* healthUI;
};
