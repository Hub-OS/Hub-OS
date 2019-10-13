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
#include "bnPlayerForm.h"

/**
 * @class ChipSelectionCust
 * @author mav
 * @date 05/05/19
 * @brief Chipcust GUI used in battle. Can be interacted through public API.
 */
class ChipSelectionCust : public SceneNode {
public:
  /**
   * @struct Bucket
   * @author mav
   * @date 05/05/19
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
  mutable sf::Sprite formItemBG;
  Animation cursorSmallAnimator;
  Animation cursorBigAnimator;
  Animation formSelectAnimator;
  Animation formCursorAnimator;
  mutable SpriteSceneNode icon;
  mutable SpriteSceneNode chipCard;
  mutable SpriteSceneNode chipNoData;
  mutable SpriteSceneNode chipSendData;
  mutable SpriteSceneNode element;
  mutable SpriteSceneNode formSelect;
  mutable SpriteSceneNode formCursor;
  sf::Shader& greyscale;
  sf::Font* labelFont;
  sf::Font* codeFont;
  mutable sf::Text smCodeLabel;
  mutable sf::Text label;
  mutable CustEmblem emblem;

  int formCursorRow;
  int selectedForm, thisFrameSelectedForm;
  std::vector<sf::Sprite> formUI;
  float formSelectQuitTimer;

  int chipCount; /*!< How many chips are listed in the GUI */
  int selectCount; /*!< How many chips the user has selected */
  int chipCap; /*!< Chips user can get */
  mutable int cursorPos; /*!< Colum of the cursor */
  mutable int cursorRow; /*!< Row of the cursor */
  bool areChipsReady; /*!< If chips have been OKd by user */
  bool isInView; /*!< If the chip cust is all the way in the player's screen */
  bool isInFormSelect; 
  bool canInteract;
  int perTurn; /*!< How many chips the player can get per turn */
  ChipFolder* folder; /*!< The loaded chip folder. @warning Will consume and delete this resource */
  Chip** selectedChips; /*!< Pointer to a list of selected chips */
  Bucket* queue; /*!< List of buckets */
  Bucket** selectQueue; /*!< List of selected buckets in order */
  ChipDescriptionTextbox chipDescriptionTextbox; /*!< Popups chip descriptions */

  double frameElapsed; /*!< delta seconds since last frame */

  std::vector<PlayerFormMeta*> forms;

public:
  /**
   * @brief Constructs chip select GUI. Creates a select queue of 8 buckets. Limits cap to 8.
   * @param _folder Pointer to ChipFolder to use. Deletes after use.
   * @param cap How many chips that can load in total. GUI only supports 8 max.
   * @param perTurn How many chips are drawn per turn. Default is 5.
   */
  ChipSelectionCust(ChipFolder* _folder, int cap, int perTurn = 5);
  
  /**
   * @brief Clears and deletes all chips and queues. Deletes folder.
   */
  ~ChipSelectionCust();

  // GUI ops
  
  /**
   * @brief Opens the textbox with chip description
   * @return true if successful. False otherwise.
   */
  bool OpenChipDescription();
  
  /**
   * @brief Continues onto the next "page" of text 
   * @return true if successful. False otherwise.
   */
  bool ContinueChipDescription();
  
  /**
   * @brief Speed up text
   * @param factor speed up amount
   * @return true if successful. False otherwise.
   */
  bool FastForwardChipDescription(double factor);
  
  /**
   * @brief Closes textbox
   * @return true if succesful. False otherwise.
   */
  bool CloseChipDescription();
  
  /**
   * @brief If in a question, selects YES
   * @return true if operation successful. False otherwise.
   */
  bool ChipDescriptionYes();
  
  /**
   * @brief If in a question, selects NO
   * @return true if operation successful. False otherwise.
   */
  bool ChipDescriptionNo();
  
  /**
   * @brief Moves chip cursor up
   * @return true if successful. False otherwise.
   */
  bool CursorUp();
  
  /**
   * @brief Moves chip cursor down
   * @return true if successful. False otherwise.
   */
  bool CursorDown();
  
  /**
   * @brief Moves chip cursor right
   * @return true if successful. False otherwise.
   */
  bool CursorRight();
  
  /**
   * @brief Moves chip cursor left
   * @return true if successful. False otherwise.
   */
  bool CursorLeft();
  
  /**
   * @brief If on a chip, adds to the selectQueue. If on the OK, sets AreChipsReady to true.
   * @return true if operation was successful. False otherwise.
   */
  bool CursorAction();
  
  /**
   * @brief Dequeues chips in FILO order. If no more chips are in selectQueue, fails.
   * @return true if operation was successful. False otherwise.
   */
  bool CursorCancel();

  /**
   * @brief Check if GUI is completely out of view from the player
   * @return true if in its final position offscreen
   */
  bool IsOutOfView();
  
  /**
   * @brief Check if GUI is completely in view for the player
   * @return true if in its final position centerscreen.
   */
  const bool IsInView();
  
  /**
   * @brief Check if the textbox is open
   * @return true if textbox is open. False otherwise.
   */
  bool IsChipDescriptionTextBoxOpen();
  
  /**
   * @brief Moves GUI by delta in screen pixels
   * @param delta offset from current position
   */
  void Move(sf::Vector2f delta);
  
  /**
   * @brief TODO: Is this still used now that we have scene nodes?
   * @return 
   */
  const sf::Vector2f GetOffset() const { return custSprite.getPosition() - sf::Vector2f(-custSprite.getTextureRect().width*2.f, 0.f); } // TODO: Get rid. See BattleScene.cpp line 241

  void SetPlayerFormOptions(const std::vector<PlayerFormMeta*> forms);

  /**
   * @brief Draws GUI and all child graphics on it
   * @param target
   * @param states
   */
  void draw(sf::RenderTarget& target, sf::RenderStates states) const;
  
  /**
   * @brief Animators cursors and textbox animations
   * @param elapsed in seconds
   */
  void Update(float elapsed);

  /**
   * @brief Confirm question selection for textbox 
   * @return true if operation was successful, false otherwise
   */
  bool ChipDescriptionConfirmQuestion();

  // Chip ops
  
  /**
   * @brief Get the next perTurn chips. Return if there are no more valid chips in the folder.
   */
  void GetNextChips();
  
  /**
   * @brief Transfer ownership from Folder through GUI to user of all selected chips in queue
   * @return List of Chip*
   */
  Chip** GetChips();
  
  /**
   * @brief Shifts all leftover chips so that the queue has non-null buckets. Points selected chips to null.
   */
  void ClearChips();
  
  /**
   * @brief Get selected chips
   * @return int
   */
  const int GetChipCount();

  const int GetSelectedFormIndex();

  const bool SelectedNewForm();

  bool CanInteract();
  
  /**
   * @brief Calls ClearChips(), resets cursor, and sets AreChipsReady are false
   */
  void ResetState();
  
  /**
   * @brief Query status if chips are selected to be used
   * @return true if chips are ready to be used in battle
   */
  bool AreChipsReady();
};

