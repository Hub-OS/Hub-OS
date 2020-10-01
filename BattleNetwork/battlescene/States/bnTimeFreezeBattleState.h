#pragma once

#include <Swoosh/Timer.h>

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

  bool showSummonText; /*!< Whether or not TFC label should appear */
  double summonTextLength; /*!< How long TFC label should stay on screen */
  bool showSummonBackdrop; /*!< Dim screen and show new backdrop if applicable */
  double showSummonBackdropLength; /*!< How long the dim should last */
  double showSummonBackdropTimer; /*!< If > 0, the state is in effect else change state */
  swoosh::Timer summonTimer; /*!< Timer for TFC label to appear at top */

  void onStart() override;
  void onEnd() override;
  void onUpdate(double elapsed) override;
  void onDraw(sf::RenderTexture& surface) override;
  const bool FadeOutBackdrop();
  const bool FadeInBackdrop();
  bool IsOver();
};
