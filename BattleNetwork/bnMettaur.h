#pragma once
#include "bnAnimatedCharacter.h"
#include "bnMobState.h"
#include "bnAI.h"
#include "bnTextureType.h"
#include "bnMobHealthUI.h"

<<<<<<< HEAD
=======
/*! \brief Basic mettaur enemy */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
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
<<<<<<< HEAD
  virtual void RefreshTexture();
  //virtual void SetAnimation(string _state, std::function<void()> onFinish = nullptr);
  //virtual void SetCounterFrame(int frame);
  virtual int GetHealth() const;
  virtual TextureType GetTextureType() const;

  void SetHealth(int _health);
  virtual int* GetAnimOffset();
  virtual const bool Hit(Hit::Properties props = Hit::DefaultProperties);
=======
  
  /**
   * @brief Takes damage and flashes white
   * @param props
   * @return true if hit, false if missed
   */
  virtual const bool Hit(Hit::Properties props = Hit::DefaultProperties);

  virtual const bool OnHit(Hit::Properties props) { return true; }

  virtual void OnDelete() { ;  }
  
  /**
   * @brief Get the hit height of this entity
   * @return const float
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
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

<<<<<<< HEAD
  static vector<int> metIDs;
  static int currMetIndex;
  int metID;

  string state;

  float hitHeight;
=======
  static vector<int> metIDs; /*!< list of mettaurs spawned to take turns */
  static int currMetIndex; /*!< current active mettaur ID */
  int metID; /*!< This mettaur's turn ID */

  float hitHeight; /*!< hit height of this mettaur */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  TextureType textureType;
};