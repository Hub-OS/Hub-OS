#pragma once
#include "bnAnimatedCharacter.h"
#include "bnMobState.h"
#include "bnMettaurIdleState.h"
#include "bnAI.h"
#include "bnTextureType.h"
#include "bnMobHealthUI.h"
#include "bnTurnOrderTrait.h"

/*! \brief Basic mettaur enemy */
class Mettaur : public AnimatedCharacter, public AI<Mettaur>, public TurnOrderTrait<Mettaur> {
  friend class MettaurIdleState;
  friend class MettaurMoveState;
  friend class MettaurAttackState;

public:
    using DefaultState = MettaurIdleState;

    /**
   * @brief Loads animations and gives itself a turn ID 
   */
  Mettaur(Rank _rank = Rank::_1);
  
  /**
   * @brief Removes its turn ID from the list of active mettaurs
   */
  virtual ~Mettaur();

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

  float hitHeight; /*!< hit height of this mettaur */
  TextureType textureType;

  DefenseRule* virusBody;
};