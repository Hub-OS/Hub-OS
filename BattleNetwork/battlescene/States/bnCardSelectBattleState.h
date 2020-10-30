#pragma once

#include "../bnBattleSceneState.h"
#include "bnCharacterTransformBattleState.h"
#include "../../bnCard.h"

#include <vector>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Font.hpp>

class Player;

/*
    \brief This state will move the cust GUI and allow the player to select new cards
*/
class CardSelectBattleState final : public BattleSceneState {
  enum class state : int {
    slidein,
    slideout,
    select
  } currState{};

  // Selection input delays
  double maxCardSelectInputCooldown; /*!< When interacting with Card Cust GUI API, delay input */
  double heldCardSelectInputCooldown; /*!< When holding the directional inputs, when does the sticky key effect trigger*/
  double cardSelectInputCooldown; /*!< Time remaining with delayed input */

  bool hasCombo{ false }; 
  bool formSelected{ false };
  int currForm{ -1 };
  int cardCount{ 0 }; /*!< Length of card list */
  float streamVolume{ -1.f };
  std::vector<Player*> tracked;
  std::vector<std::shared_ptr<TrackedFormData>> forms;
  std::shared_ptr<sf::Font> font;
  sf::Sprite mobEdgeSprite, mobBackdropSprite; /*!< name backdrop images*/
  Battle::Card** cards; /*!< List of Card* the user selects from the card cust */

  // Check if a form change was properly triggered
  void CheckFormChanges();
public:
  Battle::Card**& GetCardPtrList();
  int& GetCardListLengthAddr();
  void onStart(const BattleSceneState* last) override;
  void onUpdate(double elapsed) override;
  void onDraw(sf::RenderTexture& surface) override;
  void onEnd(const BattleSceneState* next) override;
  bool OKIsPressed();
  bool HasForm();
  const bool HasCombo();
  CardSelectBattleState(std::vector<Player*> tracked, std::vector<std::shared_ptr<TrackedFormData>> forms);
};