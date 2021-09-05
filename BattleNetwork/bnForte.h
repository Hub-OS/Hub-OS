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
  double dropCooldown;

  class MoveEffect : public Artifact {
    friend class Forte;
    double elapsed;
    static int counter;
    int index;
    std::shared_ptr<AnimationComponent> animationComponent;

  public:
    MoveEffect(Field* field);
    ~MoveEffect();

    void OnUpdate(double _elapsed) override;
    void OnDelete() override;
  };
  
  std::shared_ptr<CardAction> OnExecuteBusterAction() override final;
  std::shared_ptr<CardAction> OnExecuteChargedBusterAction() override final;
  std::shared_ptr<CardAction> OnExecuteSpecialAction() override final;
public:
  friend class PlayerControlledState;
  friend class PlayerIdleState;

  Forte();
  ~Forte();

  const float GetHeight() const override final;
  void OnUpdate(double _elapsed) override final;
  void OnDelete() override final;
  void OnSpawn(Battle::Tile& start) override final;
}; 
