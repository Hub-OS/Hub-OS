/*! \brief Forte playable net navi
 *
 * Sets health to 2000, name to "Bass", enables Float Shoe, high charge, and spawns with Aura
 */

#pragma once

#include "bnPlayer.h"
#include "bnAnimationComponent.h"
#include "bnPlayerControlledState.h"
#include "bnPlayerIdleState.h"
#include "bnAura.h"
#include "bnArtifact.h"

using sf::IntRect;

class Forte : public Player {
private:
  float dropCooldown;

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
  
  CardAction* OnExecuteBusterAction() override final;
  CardAction* OnExecuteChargedBusterAction() override final;
  CardAction* OnExecuteSpecialAction() override final;
public:
  friend class PlayerControlledState;
  friend class PlayerIdleState;

  Forte();
  ~Forte();

  const float GetHeight() const override final;
  void OnUpdate(float _elapsed) override final;
  void OnDelete() override final;
  void OnSpawn(Battle::Tile& start) override final;
}; 
