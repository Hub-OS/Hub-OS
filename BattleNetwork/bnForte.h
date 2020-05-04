/*! \brief Forte playable net navi
 *
 * Sets health to 2000, name to "Bass", enables Float Shoe, high charge, and spawns with Aura
 */

#pragma once

#include "bnPlayer.h"
#include "bnArtifact.h"
#include "bnPlayerState.h"
#include "bnTextureType.h"
#include "bnPlayerHealthUI.h"
#include "bnChargeEffectSceneNode.h"
#include "bnAnimationComponent.h"
#include "bnAI.h"
#include "bnPlayerControlledState.h"
#include "bnPlayerIdleState.h"
#include "bnPlayerHitState.h"
#include "bnNaviRegistration.h"
#include "bnAura.h"

using sf::IntRect;

class Forte : public Player {
private:
  float dropCooldown;
  Aura* aura;
  class MoveEffect : public Artifact {
    friend class Forte;
    float elapsed;
    static int counter;
    int index;
    AnimationComponent* animationComponent;

  public:
    MoveEffect(Field* field);
    ~MoveEffect();

    void OnUpdate(float _elapsed) override;
    void OnDelete() override;
  };
  
  CardAction* ExecuteBusterAction() final;
  CardAction* ExecuteChargedBusterAction() final;
  CardAction* ExecuteSpecialAction() final;
public:
  friend class PlayerControlledState;
  friend class PlayerIdleState;

  Forte();
  ~Forte();

  const float GetHeight() const final;
  void OnUpdate(float _elapsed) final;
  void OnDelete() final;
}; 
