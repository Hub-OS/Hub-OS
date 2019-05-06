#pragma once
#include "bnAnimatedCharacter.h"
#include "bnMobState.h"
#include "bnAI.h"
#include "bnTextureType.h"
#include "bnMobHealthUI.h"
#include "bnCounterHitPublisher.h"

/**
 * @class Canodumb
 * @author mav
 * @date 05/05/19
 * @file bnCanodumb.h
 * @brief Classic cannon enemy waits for opponent to be in view.
 * @warning Legacy code. Should update code.
 */
class Canodumb : public AnimatedCharacter, public AI<Canodumb> {
  friend class CanodumbIdleState;
  friend class CanodumbMoveState;
  friend class CanodumbAttackState;
  friend class CanodumbCursor;

public:
  Canodumb(Rank _rank = Character::Rank::_1);
  virtual ~Canodumb();

  /**
   * @brief When health is low, deletes. Updates AI
   * @param _elapsed
   */
  virtual void Update(float _elapsed);

  virtual const bool Hit( Hit::Properties props = Hit::DefaultProperties);
  virtual const float GetHitHeight() const;

private:
  sf::Shader* whiteout;
  sf::Shader* stun;

  MobHealthUI* healthUI;
};