#pragma once
#include "bnHoneyBomberIdleState.h"
#include "bnHoneyBomberMoveState.h"
#include "bnHoneyBomberAttackState.h"
#include "bnAI.h"
#include "bnTurnOrderTrait.h"
#include "bnTextureType.h"
#include "bnCharacter.h"

/*! \brief Basic honey comb enemy */
class HoneyBomber : public Character, public AI<HoneyBomber>, public TurnOrderTrait<HoneyBomber> {
  friend class HoneyBomberIdleState;
  friend class HoneyBomberMoveState;
  friend class HoneyBomberAttackState;

public:
  using DefaultState = HoneyBomberIdleState;

  /**
 * @brief Loads animations based on rank
 */
  HoneyBomber(Rank _rank = Rank::_1);
  ~HoneyBomber();

  /**
   * @brief Uses AI state to move around. Deletes when health is below zero.
   * @param _elapsed in seconds
   */
  void OnUpdate(float _elapsed);

  /**
   * @brief Takes damage and flashes white
   * @param props
   * @return true if hit, false if missed
   */
  const bool OnHit(const Hit::Properties props);

  void OnDelete();

  /**
   * @brief Get the hit height of this entity
   * @return const float
   */
  const float GetHeight() const override;

private:

  float hitHeight; /*!< hit height of this entity */
  SpriteProxyNode* shadow;
  DefenseRule* virusBody;
};