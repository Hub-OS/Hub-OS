#pragma once

#include "../bnBattleSceneState.h"
#include "../../bnSpriteProxyNode.h"

#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Shader.hpp>

class Mob;
class Player;

/*
    \brief This state will govern combat rules
*/
struct CombatBattleState final : public BattleSceneState {
  bool isPaused{ false };
  bool isGaugeFull{ false };
  double customProgress{ 0 };
  double customDuration{ 0 };
  Mob* mob{ nullptr };
  sf::Text pauseLabel; /*!< "PAUSE" text */
  sf::Sprite doubleDelete;
  sf::Sprite tripleDelete;
  SpriteProxyNode customBar;
  sf::Shader& customBarShader; /*!< Cust gauge shaders */
  sf::Shader& pauseShader; /*!< Dim screen */
  std::vector<Player*> tracked;

  const bool HasTimeFreeze() const;
  const bool PlayerWon() const;
  const bool PlayerLost() const;
  const bool PlayerRequestCardSelect();

  void onStart() override;
  void onEnd() override;
  void onUpdate(double elapsed) override;
  void onDraw(sf::RenderTexture& surface) override;

  CombatBattleState(Mob* mob, std::vector<Player*> tracked, double customDuration);
};