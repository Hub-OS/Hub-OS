#pragma once
#include "bnMobState.h"
#include "bnMetridIdleState.h"
#include "bnMetridMoveState.h"
#include "bnMetridAttackState.h"
#include "bnAI.h"
#include "bnTextureType.h"
#include "bnCharacter.h"

/*! \brief Basic metrid enemy */
class Metrid : public Character, public AI<Metrid> {
  friend class MetridIdleState;
  friend class MetridMoveState;
  friend class MetridAttackState;

public:
  using DefaultState = MetridIdleState;

  /**
 * @brief Loads animations based on rank
 */
  Metrid(Rank _rank = Rank::_1);
  virtual ~Metrid();

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

  const bool IsMetridTurn() const;

  /**
   * @brief Passes control to the next mettaur
   */
  void NextMetridTurn();

  static vector<int> metIDs; /*!< list of metrids spawned to take turns */
  static int currMetIndex; /*!< current active metrid ID */
  int metID; /*!< This metrid's turn ID */

  float hitHeight; /*!< hit height of this entity */

  DefenseRule* virusBody;
};