#pragma once
#include "bnAnimatedCharacter.h"
#include "bnMobState.h"
#include "bnAI.h"
#include "bnTextureType.h"
#include "bnMobHealthUI.h"

class Canodumb : public AnimatedCharacter, public AI<Canodumb> {
  friend class CanodumbIdleState;
  friend class CanodumbMoveState;
  friend class CanodumbAttackState;
  friend class CanodumbCursor;

public:
  Canodumb(Rank _rank = Character::Rank::_1);
  virtual ~Canodumb(void);

  virtual void Update(float _elapsed);
  virtual vector<Drawable*> GetMiscComponents();
  int* GetAnimOffset();
  virtual const bool Hit(int _damage, HitProperties props = Entity::DefaultHitProperties);
  virtual const float GetHitHeight() const;

private:
  sf::Shader* whiteout;
  sf::Shader* stun;

  MobHealthUI* healthUI;
};