#pragma once
#include "bnCharacter.h"
#include "bnAI.h"
#include "bnTextureType.h"
#include "bnMobHealthUI.h"
#include "bnCounterHitPublisher.h"
#include "bnCanodumbIdleState.h"

/**
 * @class Canodumb
 * @author mav
 * @date 05/05/19
 * @brief Classic cannon enemy waits for opponent to be in view.
 */
class Canodumb : public Character, public AI<Canodumb> {
  friend class CanodumbIdleState;
  friend class CanodumbAttackState;
  friend class CanodumbCursor;

  float hitHeight;
  std::shared_ptr<DefenseRule> virusBody{ nullptr };
  std::shared_ptr<AnimationComponent> animation{ nullptr };
public:
  using DefaultState = CanodumbIdleState;

  Canodumb(Rank _rank = Character::Rank::_1);
  ~Canodumb();

  /**
   * @brief When health is low, deletes. Updates AI
   * @param _elapsed
   */
  void OnUpdate(double _elapsed);

  const float GetHeight() const;

  void OnDelete();
};