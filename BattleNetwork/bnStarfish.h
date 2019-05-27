<<<<<<< HEAD
=======
/*! \brief Starfish is Aqua type and spawns bubbles */

>>>>>>> b486e21e11627262088deae73097eaa7af56791c
#pragma once
#include "bnAnimatedCharacter.h"
#include "bnMobState.h"
#include "bnAI.h"
#include "bnTextureType.h"
#include "bnMobHealthUI.h"

class Starfish : public AnimatedCharacter, public AI<Starfish> {
  friend class StarfishIdleState;
  friend class StarfishAttackState;

public:
  Starfish(Rank _rank = Rank::_1);
  virtual ~Starfish(void);

<<<<<<< HEAD
  virtual void Update(float _elapsed);
  virtual void RefreshTexture();
  //virtual void SetAnimation(string _state, std::function<void()> onFinish = nullptr);
  //virtual void SetCounterFrame(int frame);
  virtual int GetHealth() const;

  void SetHealth(int _health);
  virtual int* GetAnimOffset();
  virtual const bool Hit( Hit::Properties props = Hit::DefaultProperties);
=======
  /**
   * @brief Updates health ui, AI, and super classes
   * @param _elapsed in seconds
   */
  virtual void Update(float _elapsed);
  
  // TODO: remove this
  virtual const bool Hit( Hit::Properties props = Hit::DefaultProperties);

  virtual const bool OnHit(Hit::Properties props) { return true;  }

  virtual void OnDelete() { ; }
  
  /**
   * @brief Set the hit height for projectiles to play effects at the correct position
   * @return Y offset
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  virtual const float GetHitHeight() const;

private:
  sf::Shader* whiteout;
  sf::Shader* stun;
<<<<<<< HEAD

  string state;

=======
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  float hitHeight;
  bool hit;
  TextureType textureType;
  MobHealthUI* healthUI;
};
