#pragma once

#include "../bnBattleSceneState.h"
#include "../../bnAnimation.h"

#include <SFML/Graphics/Sprite.hpp>

class Player;

/*
    \brief This state handles transformations
*/
struct CharacterTransformBattleState final : public BattleSceneState {
  using TrackedFormData = std::pair<Player*, int>;

  int lastSelectedForm{ -1 };
  double backdropTimer{ 0 };
  double backdropLength{ 0 };
  bool isLeavingFormChange{ false };
  Animation shineAnimation;
  sf::Sprite shine;
  std::vector<TrackedFormData> tracking;

  bool IsFinished();
  void onStart() override;
  void onUpdate(double elapsed) override;

  CharacterTransformBattleState(std::vector<TrackedFormData> tracking);
};