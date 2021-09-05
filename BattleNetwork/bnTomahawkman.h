/*! \brief Tomahawkman playable net navi
 *
 * Sets health to 1200, names "Tomahawk", and charge uses tomahawk card action
 */

#pragma once

#include "bnPlayer.h"
#include "bnPlayerControlledState.h"
#include "bnPlayerIdleState.h"

using sf::IntRect;

class Tomahawkman : public Player {
private:
  std::shared_ptr<CardAction> OnExecuteBusterAction() override final;
  std::shared_ptr<CardAction> OnExecuteChargedBusterAction() override final;
  std::shared_ptr<CardAction> OnExecuteSpecialAction() override final;
public:
  friend class PlayerControlledState;
  friend class PlayerIdleState;

  Tomahawkman();
  ~Tomahawkman();

  const float GetHeight() const final;
  void OnUpdate(double _elapsed) final;
  void OnDelete() final;
};
