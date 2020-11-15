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
  CardAction* OnExecuteBusterAction() override final;
  CardAction* OnExecuteChargedBusterAction() override final;
  CardAction* OnExecuteSpecialAction() override final;
public:
  friend class PlayerControlledState;
  friend class PlayerIdleState;

  Tomahawkman();
  ~Tomahawkman();

  const float GetHeight() const final;
  void OnUpdate(float _elapsed) final;
  void OnDelete() final;
};
