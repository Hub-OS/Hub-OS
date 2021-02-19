#pragma once

#include "bnResourceHandle.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnCardFolder.h"
#include "bnGame.h"
#include "bnAnimation.h"
#include "bnCardDescriptionTextbox.h"
#include "bnCustEmblem.h"
#include "bnSceneNode.h"
#include "bnPlayerForm.h"

#include <SFML/Graphics.hpp>
#include <Swoosh/Ease.h>

/**
 * @class CardSelectionCust
 * @author mav
 * @date 05/05/19
 * @brief Cardcust widget used in battle. Can be interacted through public API.
 */
class CardSelectionCust : public SceneNode, public ResourceHandle {
public:
  /**
   * @struct Bucket
   * @author mav
   * @date 05/05/19
   * @brief Card state bucket
   * 
   * A card may be voided, staged, queued
   */
  struct Bucket {
    Battle::Card* data;
    enum class state : short {
      voided = 0,
      staged,
      queued
    } state;
  };

private:
  mutable sf::Sprite custSprite;
  mutable sf::Sprite custDarkCardOverlay, custMegaCardOverlay, custGigaCardOverlay;
  mutable sf::Sprite cursorSmall; // animated
  mutable sf::Sprite cursorBig;   // animated
  mutable sf::Sprite cardLock;
  mutable sf::Sprite formItemBG;
  mutable sf::Sprite currentFormItem;
  mutable sf::Sprite lockedInFormItem;
  Animation cursorSmallAnimator;
  Animation cursorBigAnimator;
  Animation formSelectAnimator;
  Animation formCursorAnimator;
  mutable SpriteProxyNode icon;
  mutable SpriteProxyNode cardCard;
  mutable SpriteProxyNode cardNoData;
  mutable SpriteProxyNode cardSendData;
  mutable SpriteProxyNode element;
  mutable SpriteProxyNode formSelect;
  mutable SpriteProxyNode formCursor;
  sf::Shader& greyscale;
  Font labelFont;
  Font codeFont, codeFont2;
  mutable Text smCodeLabel;
  mutable Text label;
  mutable CustEmblem emblem;

  int formCursorRow; //!< Cursor row
  int selectedFormRow; //!< selected form row
  int selectedFormIndex; //!< Form Index of selection 
  int lockedInFormIndex; //!< What the card cust has locked our selection in as
  std::vector<sf::Sprite> formUI;
  double formSelectQuitTimer;
  bool playFormSound;

  int cardCount; /*!< How many cards are listed in the GUI */
  int selectCount; /*!< How many cards the user has selected */
  int cardCap; /*!< Cards user can get */
  mutable int cursorPos; /*!< Colum of the cursor */
  mutable int cursorRow; /*!< Row of the cursor */
  bool areCardsReady; /*!< If cards have been OKd by user */
  bool isInView; /*!< If the card cust is all the way in the player's screen */
  bool isInFormSelect; 
  bool canInteract;
  bool isDarkCardSelected;
  float darkCardShadowAlpha;
  int perTurn; /*!< How many cards the player can get per turn */
  CardFolder* folder; /*!< The loaded card folder. @warning Will consume and delete this resource */
  Battle::Card** selectedCards; /*!< Pointer to a list of selected cards */
  Bucket* queue; /*!< List of buckets */
  Bucket** selectQueue; /*!< List of selected buckets in order */
  CardDescriptionTextbox cardDescriptionTextbox; /*!< Popups card descriptions */

  double frameElapsed; /*!< delta seconds since last frame */

  std::vector<PlayerFormMeta*> forms;

public:
  /**
   * @brief Constructs card select GUI. Creates a select queue of 8 buckets. Limits cap to 8.
   * @param _folder Pointer to CardFolder to use. Deletes after use.
   * @param cap How many cards that can load in total. GUI only supports 8 max.
   * @param perTurn How many cards are drawn per turn. Default is 5.
   */
  CardSelectionCust(CardFolder* _folder, int cap, int perTurn = 5);
  
  /**
   * @brief Clears and deletes all cards and queues. Deletes folder.
   */
  ~CardSelectionCust();

  // GUI ops
  
  /**
   * @brief Opens the textbox with card description
   * @return true if successful. False otherwise.
   */
  bool OpenCardDescription();
  
  /**
   * @brief Continues onto the next "page" of text 
   * @return true if successful. False otherwise.
   */
  bool ContinueCardDescription();
  
  /**
   * @brief Speed up text
   * @param factor speed up amount
   * @return true if successful. False otherwise.
   */
  bool FastForwardCardDescription(double factor);
  
  /**
   * @brief Closes textbox
   * @return true if succesful. False otherwise.
   */
  bool CloseCardDescription();
  
  /**
   * @brief If in a question, selects YES
   * @return true if operation successful. False otherwise.
   */
  bool CardDescriptionYes();
  
  /**
   * @brief If in a question, selects NO
   * @return true if operation successful. False otherwise.
   */
  bool CardDescriptionNo();
  
  /**
   * @brief Moves card cursor up
   * @return true if successful. False otherwise.
   */
  bool CursorUp();
  
  /**
   * @brief Moves card cursor down
   * @return true if successful. False otherwise.
   */
  bool CursorDown();
  
  /**
   * @brief Moves card cursor right
   * @return true if successful. False otherwise.
   */
  bool CursorRight();
  
  /**
   * @brief Moves card cursor left
   * @return true if successful. False otherwise.
   */
  bool CursorLeft();
  
  /**
   * @brief If on a card, adds to the selectQueue. If on the OK, sets AreCardsReady to true.
   * @return true if operation was successful. False otherwise.
   */
  bool CursorAction();
  
  /**
   * @brief Dequeues cards in FILO order. If no more cards are in selectQueue, fails.
   * @return true if operation was successful. False otherwise.
   */
  bool CursorCancel();

  /**
  * @brief Moves the cursor to the OK button
  * @return true if operation was successful. False otherwise
  */
  bool CursorSelectOK();

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
  bool IsCardDescriptionTextBoxOpen();
  
  const bool IsDarkCardSelected();

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
  void LockInPlayerFormSelection();
  void ResetPlayerFormSelection();
  void ErasePlayerFormOption(size_t index);

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
  void Update(double elapsed);

  /**
   * @brief Confirm question selection for textbox 
   * @return true if operation was successful, false otherwise
   */
  bool CardDescriptionConfirmQuestion();

  // Card ops
  
  /**
   * @brief Get the next perTurn cards. Return if there are no more valid cards in the folder.
   */
  void GetNextCards();
  
  /**
   * @brief Transfer ownership from Folder through GUI to user of all selected cards in queue
   * @return List of Card*
   */
  Battle::Card** GetCards();
  
  /**
   * @brief Shifts all leftover cards so that the queue has non-null buckets. Points selected cards to null.
   */
  void ClearCards();
  
  /**
   * @brief Get selected cards
   * @return int
   */
  const int GetCardCount();

  const int GetSelectedFormIndex();

  const bool SelectedNewForm();

  bool CanInteract();
  
  /**
   * @brief Calls ClearCards(), resets cursor, and sets AreCardsReady are false
   */
  void ResetState();
  
  /**
   * @brief Query status if cards are selected to be used
   * @return true if cards are ready to be used in battle
   */
  bool AreCardsReady();
};

