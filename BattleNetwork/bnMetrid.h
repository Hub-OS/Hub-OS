#pragma once
#include "bnMetridIdleState.h"
#include "bnMetridMoveState.h"
#include "bnMetridAttackState.h"
#include "bnAI.h"
#include "bnTurnOrderTrait.h"
#include "bnTextureType.h"
#include "bnCharacter.h"

/*! \brief Basic metrid enemy */
class Metrid : public Character, public AI<Metrid>, public TurnOrderTrait<Metrid> {
  friend class MetridIdleState;
  friend class MetridMoveState;
  friend class MetridAttackState;

public:
  using DefaultState = MetridIdleState;

  /**
 * @brief Loads animations based on rank
 */
  Metrid(Rank _rank = Rank::_1);
  ~Metrid();

  /**
   * @brief Uses AI state to move around. Deletes when health is below zero.
   * @param _elapsed in seconds
   */
  void OnUpdate(double _elapsed);

  void OnDelete();

  /**
   * @brief Get the hit height of this entity
   * @return const float
   */
  const float GetHeight() const;

private:

  float hitHeight; /*!< hit height of this entity */

  DefenseRule* virusBody;
};