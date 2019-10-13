#pragma once
#include "bnMobState.h"
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
  virtual ~HoneyBomber();

  /**
   * @brief Uses AI state to move around. Deletes when health is below zero.
   * @param _elapsed in seconds
   */
  virtual void OnUpdate(float _elapsed);

  /**
   * @brief Takes damage and flashes white
   * @param props
   * @return true if hit, false if missed
   */
  virtual const bool OnHit(const Hit::Properties props);

  virtual void OnDelete();

  /**
   * @brief Get the hit height of this entity
   * @return const float
   */
  virtual const float GetHitHeight() const;

private:

  float hitHeight; /*!< hit height of this entity */
  SpriteSceneNode* shadow;
  DefenseRule* virusBody;
};