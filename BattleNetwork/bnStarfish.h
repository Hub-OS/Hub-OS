/*! \brief Starfish is Aqua type and spawns bubbles */

#pragma once
#include "bnAnimatedCharacter.h"
#include "bnMobState.h"
#include "bnAI.h"
#include "bnTextureType.h"
#include "bnMobHealthUI.h"

class Starfish : public AnimatedCharacter, public AI<Starfish> {
  friend class StarfishIdleState;
  friend class StarfishAttackState;
  using DefaultState = StarfishIdleState;

public:
  Starfish(Rank _rank = Rank::_1);
  virtual ~Starfish(void);

  /**
   * @brief Updates health ui, AI, and super classes
   * @param _elapsed in seconds
   */
  virtual void Update(float _elapsed);

  virtual const bool OnHit(const Hit::Properties props);

  virtual void OnDelete();
  
  /**
   * @brief Set the hit height for projectiles to play effects at the correct position
   * @return Y offset
   */
  virtual const float GetHitHeight() const;

private:
  float hitHeight;
  TextureType textureType;
  MobHealthUI* healthUI;
};
