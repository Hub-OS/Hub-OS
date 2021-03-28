#pragma once
#include <vector>
#include <SFML/Graphics/Sprite.hpp>

#include "../bnBattleSceneState.h"
#include "bnCharacterTransformBattleState.h"
#include "../../bnCard.h"
#include "../../bnFont.h"

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

  bool pvpMode{ false };
  bool hasNewChips{ false }; 
  bool formSelected{ false };
  int currForm{ -1 };
  int cardCount{ 0 }; /*!< Length of card list */
  float streamVolume{ -1.f };
  std::vector<Player*>& tracked;
  std::vector<std::shared_ptr<TrackedFormData>>& forms;
  Font font;
  sf::Sprite mobEdgeSprite, mobBackdropSprite; /*!< name backdrop images*/
  Battle::Card** cards; /*!< List of Card* the user selects from the card cust */

  // Check if a form change was properly triggered
  void CheckFormChanges();
public:
  CardSelectBattleState(std::vector<Player*>& tracked, std::vector<std::shared_ptr<TrackedFormData>>& forms);

  Battle::Card**& GetCardPtrList();
  int& GetCardListLengthAddr();
  void onStart(const BattleSceneState* last) override;
  void onUpdate(double elapsed) override;
  void onDraw(sf::RenderTexture& surface) override;
  void onEnd(const BattleSceneState* next) override;
  void EnablePVPMode();
  bool OKIsPressed();
  bool HasForm();
  bool RequestedRetreat();
  const bool SelectedNewChips();
  void ResetSelectedForm();
};