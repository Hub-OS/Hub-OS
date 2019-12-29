/*! \brief Starfish is Aqua type and spawns bubbles */

#pragma once
#include "bnAnimatedCharacter.h"
#include "bnMobState.h"
#include "bnAI.h"
#include "bnTextureType.h"
#include "bnStarfishIdleState.h"

class Starfish : public AnimatedCharacter, public AI<Starfish> {
  friend class StarfishIdleState;
  friend class StarfishAttackState;

public:
  using DefaultState = StarfishIdleState;

  Starfish(Rank _rank = Rank::_1);
  ~Starfish();

  /**
   * @brief Updates health ui, AI, and super classes
   * @param _elapsed in seconds
   */
  void OnUpdate(float _elapsed);

  const bool OnHit(const Hit::Properties props);

  void OnDelete();
  
  /**
   * @brief Set the hit height for projectiles to play effects at the correct position
   * @return Y offset
   */
  const float GetHeight() const;

private:
  float hitHeight;
  TextureType textureType;
  DefenseRule* virusBody;
};
