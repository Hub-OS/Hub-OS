/*! \brief Protoman playable net navi
 *
 * Sets health to 1200, names "Protoman", and charge uses wide sword
 */

#pragma once

#include "bnPlayer.h"
#include "bnPlayerControlledState.h"
#include "bnPlayerIdleState.h"
 
using sf::IntRect;

class Protoman : public Player {
private:
  std::shared_ptr<CardAction> OnExecuteBusterAction() override final;
  std::shared_ptr<CardAction> OnExecuteChargedBusterAction() override final;
  std::shared_ptr<CardAction> OnExecuteSpecialAction() override final;
public:
  friend class PlayerControlledState;
  friend class PlayerIdleState;

  Protoman();
  ~Protoman();

  const float GetHeight() const final;
  void OnUpdate(double _elapsed) final;
  void OnDelete() final;
};
