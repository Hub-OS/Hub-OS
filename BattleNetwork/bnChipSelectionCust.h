#pragma once

#include "Swoosh/Ease.h"

#include <SFML/Graphics.hpp>
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnChipFolder.h"
#include "bnEngine.h"
#include "bnAnimation.h"
#include "bnChipDescriptionTextbox.h"
#include "bnCustEmblem.h"
#include "bnSceneNode.h"

/**
 * @class ChipSelectionCust
 * @author mav
 * @date 05/05/19
 * @file bnChipSelectionCust.h
 * @brief Chipcust GUI used in battle. Can be interacted through public API.
 */
class ChipSelectionCust : public SceneNode {
public:
  /**
   * @class Bucket
   * @author mav
   * @date 05/05/19
   * @file bnChipSelectionCust.h
   * @brief Chip state bucket
   * 
   * A chip may be SELECTED, QUEUED, AVAILABLE
   */
  struct Bucket {
    Chip* data;
    short state;
  };

private:
  // TODO: take mutable attr out
  mutable sf::Sprite custSprite;
  mutable sf::Sprite cursorSmall; // animated
  mutable sf::Sprite cursorBig;   // animated
  mutable sf::Sprite chipLock;
  Animation cursorSmallAnimator;
  Animation cursorBigAnimator;
  mutable SpriteSceneNode icon;
  mutable SpriteSceneNode chipCard;
  mutable SpriteSceneNode chipNoData;
  mutable SpriteSceneNode chipSendData;
  mutable SpriteSceneNode element;
  sf::Shader& greyscale;
  sf::Font* labelFont;
  sf::Font* codeFont;
  mutable sf::Text smCodeLabel;
  mutable sf::Text label;
  mutable CustEmblem emblem;

  int chipCount; /*!< How many chips are listed in the GUI */
  int selectCount; /*!< How many chips the user has selected */
  int chipCap; /*!< Chips user can get */
  mutable int cursorPos; /*!< Colum of the cursor */
  mutable int cursorRow; /*!< Row of the cursor */
  bool areChipsReady; /*!< If chips have been OKd by user */
  bool isInView; /*!< If the chip cust is all the way in the player's screen */
  int perTurn; /*!< How many chips the player can get per turn */
  ChipFolder* folder; /*!< The loaded chip folder. @warning Will consume and delete this resource */
  Chip** selectedChips; /*!< Pointer to a list of selected chips */
  Bucket* queue;
  Bucket** selectQueue;
  ChipDescriptionTextbox chipDescriptionTextbox; /*!< Popups chip descriptions */

  double frameElapsed; /*!< delta seconds since last frame */

public:
  ChipSelectionCust(ChipFolder* _folder, int, int perTurn = 5);
  ~ChipSelectionCust();

  // GUI ops
  bool OpenChipDescription();
  bool ContinueChipDescription();
  bool FastForwardChipDescription(double factor);
  bool CloseChipDescription();
  bool ChipDescriptionYes();
  bool ChipDescriptionNo();
  bool CursorUp();
  bool CursorDown();
  bool CursorRight();
  bool CursorLeft();
  bool CursorAction();
  bool CursorCancel();

  bool IsOutOfView();
  const bool IsInView();
  bool IsChipDescriptionTextBoxOpen();
  void Move(sf::Vector2f delta);
  const sf::Vector2f GetOffset() const { return custSprite.getPosition() - sf::Vector2f(-custSprite.getTextureRect().width*2.f, 0.f); } // TODO: Get rid. See BattleScene.cpp line 241
  
  void draw(sf::RenderTarget& target, sf::RenderStates states) const;
  void Update(float elapsed);

  bool ChipDescriptionConfirmQuestion();

  // Chip ops
  void GetNextChips();
  Chip** GetChips();
  void ClearChips();
  const int GetChipCount();
  void ResetState();
  bool AreChipsReady();
};

