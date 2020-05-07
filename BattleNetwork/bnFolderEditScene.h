#pragma once
#include <Swoosh/Ease.h>
#include <Swoosh/Activity.h>

#include "bnCamera.h"
#include "bnInputManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnEngine.h"
#include "bnAnimation.h"
#include "bnLanBackground.h"
#include "bnCardFolder.h"

/**
 * @class FolderEditScene
 * @author mav
 * @date 04/05/19
 * @brief Edit folder contents and select from card pool
 * @important the games, the card pool card count is not shared by folders
 * 
 * User can select card and switch to the card pool on the right side of the scene to select cards
 * to swap out for. 
 * 
 * Before leaving the user is prompted to save changes
 */
 
class FolderEditScene : public swoosh::Activity {
private:
  enum class ViewMode : int {
    FOLDER,
    PACK
  };

  /**
  * @class PackBucket
  * @brief Cards in a pack avoid listing duplicates by bundling them in a counted bucket 
  * 
  * Users can select up to all of the cards in a bucket. The bucket will remain in the list but at 0. 
  */
  class PackBucket {
  private:
    unsigned size;
    unsigned maxSize;
    Battle::Card info;

  public:
    PackBucket(unsigned size, Battle::Card info) : size(size), maxSize(size), info(info) { }
    ~PackBucket() { }

    const bool IsEmpty() const { return size == 0; }
    const bool GetCard(Battle::Card& copy) { if (IsEmpty()) return false; else copy = Battle::Card(info); size--;  return true; }
    void AddCard() { size++; size = std::min(size, maxSize);  }
    const Battle::Card& ViewCard() const { return info; }
    const unsigned GetCount() const { return size; }
  };

  /**
  * @class FolderSlot
  * @brief A selectable row in the folder to place new cards. When removing cards, an empty slot is left behind
  */
  class FolderSlot {
  private:
    bool occupied;
    Battle::Card info;
  public:
    void AddCard(Battle::Card other) {
      info = other;
      occupied = true;
    }

    const bool GetCard(Battle::Card& copy) {
      if (!occupied) return false;

      copy = Battle::Card(info);
      occupied = false;

      info = Battle::Card(); // null card

      return true;
    }

    const bool IsEmpty() const {
      return !occupied;
    }

    const Battle::Card& ViewCard() {
      return info;
    }
  };

  void ExcludeFolderDataFromPack();
  void PlaceFolderDataIntoCardSlots();
  void PlaceLibraryDataIntoBuckets();
  void WriteNewFolderData();

private:
  std::vector<FolderSlot> folderCardSlots; /*!< Rows in the folder that can be inserted with cards or replaced */
  std::vector<PackBucket> packCardBuckets; /*!< Rows in the pack that represent how many of a card are left */
  bool hasFolderChanged; /*!< Flag if folder needs to be saved before quitting screen */
  Camera camera;
  CardFolder& folder;

  // Menu name font
  std::shared_ptr<sf::Font> font;
  sf::Text* menuLabel;

  // Selection input delays
  double maxSelectInputCooldown; // half of a second
  double selectInputCooldown;

  // Card UI font
  std::shared_ptr<sf::Font> cardFont;
  sf::Text *cardLabel;

  std::shared_ptr<sf::Font> numberFont;
  sf::Text *numberLabel;

  // Card description font
  std::shared_ptr<sf::Font> cardDescFont;
  sf::Text* cardDesc;

  // folder menu graphic
  sf::Sprite bg;
  sf::Sprite folderDock, packDock;
  sf::Sprite scrollbar;
  sf::Sprite cardHolder;
  sf::Sprite element;
  sf::Sprite folderCursor, folderSwapCursor;
  sf::Sprite packCursor, packSwapCursor;
  sf::Sprite folderNextArrow;
  sf::Sprite packNextArrow;
  sf::Sprite folderCardCountBox;
  sf::Sprite mbPlaceholder;

  // Current card graphic data
  sf::Sprite card;
  sf::IntRect cardSubFrame;
  sf::Sprite cardIcon;
  swoosh::Timer cardRevealTimer;
  swoosh::Timer easeInTimer;

  struct CardView {
    int maxCardsOnScreen;
    int currCardIndex;
    int lastCardOnScreen; // index
    int prevIndex; // for effect
    int numOfCards;
    int swapCardIndex; // -1 for unselected, otherwise ID
  } folderView, packView;

  ViewMode currViewMode;
  ViewMode prevViewMode;

  double totalTimeElapsed;
  double frameElapsed;
 
  bool canInteract;

#ifdef __ANDROID__
  bool canSwipe;
  bool touchStart;

  int touchPosX;
  int touchPosStartX;

  bool releasedB;

  void StartupTouchControls();
  void ShutdownTouchControls();
#endif

  void DrawFolder();
  void DrawLibrary();

public:
  std::string FormatCardDesc(const std::string&& desc);

  virtual void onStart();
  virtual void onUpdate(double elapsed);
  virtual void onLeave();
  virtual void onExit();
  virtual void onEnter();
  virtual void onResume();
  virtual void onDraw(sf::RenderTexture& surface);
  virtual void onEnd();

  FolderEditScene(swoosh::ActivityController&, CardFolder& folder);
  virtual ~FolderEditScene();
};
