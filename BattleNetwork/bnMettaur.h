#pragma once
#include "bnAnimatedCharacter.h"
#include "bnMobState.h"
#include "bnAI.h"
#include "bnTextureType.h"
#include "bnMobHealthUI.h"

/*! \brief Basic mettaur enemy */
class Mettaur : public AnimatedCharacter, public AI<Mettaur> {
  friend class MettaurIdleState;
  friend class MettaurMoveState;
  friend class MettaurAttackState;

public:
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
  virtual void Update(float _elapsed);
  
  /**
   * @brief Takes damage and flashes white
   * @param props
   * @return true if hit, false if missed
   */
  virtual const bool Hit(Hit::Properties props = Hit::DefaultProperties);
  
  /**
   * @brief Get the hit height of this entity
   * @return const float
   */
  virtual const float GetHitHeight() const;

private:
  /**
   * @brief Used in states, if this mettaur is allowed to move 
   * @return 
   */
  const bool IsMettaurTurn() const;

  /**
   * @brief Passes control to the next mettaur
   */
  void NextMettaurTurn();

  sf::Shader* whiteout;
  sf::Shader* stun;

  static vector<int> metIDs; /*!< list of mettaurs spawned to take turns */
  static int currMetIndex; /*!< current active mettaur ID */
  int metID; /*!< This mettaur's turn ID */

  float hitHeight; /*!< hit height of this mettaur */
  TextureType textureType;
};