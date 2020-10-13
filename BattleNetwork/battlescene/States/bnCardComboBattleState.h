#pragma once

#include <Swoosh/Timer.h>
#include <memory>

#include "../bnBattleSceneState.h"
#include "../../bnSelectedCardsUI.h"
#include "../../bnPA.h"

/*
    This state spells out the combo steps
 */
struct CardComboBattleState final : public BattleSceneState {

  bool isPAComplete{ false };
  bool advanceSoundPlay{ false };
  int hasPA{ -1 };
  int paStepIndex{ 0 };
  int* cardCountPtr{ nullptr };
  double elapsed{ 0 };
  double listStepCooldown{ 0 };
  double listStepCounter{ 0 };
  double PAStartLength{ 0 }; /*!< Total time to animate PA */
  PA& programAdvance; /*!< PA object loads PA database and returns matching PA card from input */
  PA::Steps paSteps; /*!< Matching steps in a PA */
  swoosh::Timer PAStartTimer; /*!< Time to scale the PA graphic */
  sf::Sprite programAdvanceSprite;
  std::shared_ptr<sf::Font> font;
  SelectedCardsUI& ui;
  Battle::Card** cards{ nullptr };

  CardComboBattleState(SelectedCardsUI& ui, PA& programAdvance);
  void ShareCardList(Battle::Card** cards, int* listLengthPtr);
  void onStart() override;
  void onEnd() override;
  void onUpdate(double elapsed) override;
  void onDraw(sf::RenderTexture& surface) override;
  bool IsDone();
};