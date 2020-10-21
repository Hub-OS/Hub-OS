#pragma once

#include "../bnBattleSceneState.h"
#include "../../bnAnimation.h"

#include <SFML/Graphics/Sprite.hpp>

class Player;

/**
  @brief Tracks form data so the card select knows when or when not to animate the player
*/
struct TrackedFormData {
  Player* player{ nullptr };
  int selectedForm{ -1 };
  bool animationComplete{ false };
};
/*
    \brief This state handles transformations
*/
class CharacterTransformBattleState final : public BattleSceneState {
public:
  enum class state : int {
    fadein = 0,
    animate,
    fadeout
  } currState{ state::fadein };

private:
  int lastSelectedForm{ -1 };
  double backdropInc{ 4.25 }; //!< alpha increase per frame (max 255)
  double frameElapsed{ 0 };
  sf::Sprite shine;
  std::vector<std::shared_ptr<TrackedFormData>> tracking;
  std::vector<Animation> shineAnimations;

  const bool FadeInBackdrop();
  const bool FadeOutBackdrop();
  void UpdateAnimation(double elapsed);

public:
  bool IsFinished();
  void onStart() override;
  void onUpdate(double elapsed) override;
  void onEnd() override;
  void onDraw(sf::RenderTexture&);

  CharacterTransformBattleState(const std::vector<std::shared_ptr<TrackedFormData>> tracking);
};