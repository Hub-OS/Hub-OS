#pragma once

#include "../bnBattleSceneState.h"
#include "../../bnCard.h"

#include <vector>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Font.hpp>

class Player;

/*
    \brief This state will move the cust GUI and allow the player to select new cards
*/
class CardSelectBattleState final : public BattleSceneState {
  enum state {
    slidein,
    slideout,
    select
  } currState{ slidein };

  // Selection input delays
  double maxCardSelectInputCooldown; /*!< When interacting with Card Cust GUI API, delay input */
  double heldCardSelectInputCooldown; /*!< When holding the directional inputs, when does the sticky key effect trigger*/
  double cardSelectInputCooldown; /*!< Time remaining with delayed input */
  int cardCount; /*!< Length of card list */
  float streamVolume{ -1.f };
  std::vector<Player*> tracked;
  sf::Font font;
  sf::Sprite mobEdgeSprite, mobBackdropSprite; /*!< name backdrop images*/
  Battle::Card** cards; /*!< List of Card* the user selects from the card cust */

public:
  Battle::Card** GetCardPtrList();
  int& GetCardListLengthAddr();
  void onStart() override;
  void onUpdate(double elapsed) override;
  void onDraw(sf::RenderTexture& surface) override;
  bool OKIsPressed();
  CardSelectBattleState(std::vector<Player*> tracked);
};