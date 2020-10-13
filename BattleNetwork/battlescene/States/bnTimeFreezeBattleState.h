#pragma once

#include <Swoosh/Timer.h>
#include <SFML/Graphics/Font.hpp>

#include "../bnBattleSceneState.h"

/* 
    \brief This state governs transitions and combat rules while in TF
*/
struct TimeFreezeBattleState final : public BattleSceneState {
  enum class state : int {
    fadein = 0,
    animate,
    fadeout
  } currState{ state::fadein };

  bool showSummonText{ false }; /*!< Whether or not TFC label should appear */
  bool showSummonBackdrop{ false }; /*!< Dim screen and show new backdrop if applicable */
  double summonTextLength{ 1.0 }; /*!< How long TFC label should stay on screen */
  double showSummonBackdropLength{ 1.0 }; /*!< How long the dim should last */
  double showSummonBackdropTimer{ 1.0 }; /*!< If > 0, the state is in effect else change state */
  sf::Font font;

  swoosh::Timer summonTimer; /*!< Timer for TFC label to appear at top */

  void onStart() override;
  void onEnd() override;
  void onUpdate(double elapsed) override;
  void onDraw(sf::RenderTexture& surface) override;
  void ExecuteSummons();
  const bool FadeOutBackdrop();
  const bool FadeInBackdrop();
  bool IsOver();
};
