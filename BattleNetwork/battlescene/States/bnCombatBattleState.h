#pragma once

#include "../bnBattleSceneState.h"
#include "../../bnSpriteProxyNode.h"
#include "../../bnCardUseListener.h"

#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Shader.hpp>

class Mob;
class Player;

namespace Battle {
  class Card;
}

/*
    \brief This state will govern combat rules
*/
struct CombatBattleState final : public BattleSceneState, public CardActionUseListener {
  bool canPause{ true };
  bool isPaused{ false };
  bool isGaugeFull{ false };
  bool hasTimeFreeze{ false };
  bool clearedMob{ false };
  double customProgress{ 0 };
  double customDuration{ 0 };
  Mob* mob{ nullptr };
  sf::Sprite pause;
  sf::Sprite doubleDelete;
  sf::Sprite tripleDelete;
  sf::Sprite counterHit;
  SpriteProxyNode customBar;
  sf::Shader* customBarShader; /*!< Cust gauge shaders */
  sf::Shader* pauseShader; /*!< Dim screen */
  std::vector<Player*>& tracked;
  std::vector<const BattleSceneState*> subcombatStates;
  const bool HasTimeFreeze() const;
  const bool PlayerWon() const;
  const bool PlayerLost() const;
  const bool PlayerRequestCardSelect();
  void EnablePausing(bool enable);

  void onStart(const BattleSceneState* last) override;
  void onEnd(const BattleSceneState* next) override;
  void onUpdate(double elapsed) override;
  void onDraw(sf::RenderTexture& surface) override;
  void OnCardActionUsed(CardAction* action, uint64_t timestamp) override;
  const bool HandleNextRoundSetup(const BattleSceneState* state);

  CombatBattleState(Mob* mob, std::vector<Player*>& tracked, double customDuration);
};