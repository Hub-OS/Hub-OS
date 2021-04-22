#pragma once
#include <Swoosh/Ease.h>

#include "bnScene.h"
#include "bnCamera.h"
#include "bnInputManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnGame.h"
#include "bnAnimation.h"
#include "bnLanBackground.h"
#include "bnCardFolder.h"
#include "bnText.h"

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
 
class FolderEditScene : public Scene {
private:
  enum class ViewMode : int {
    folder,
    pool
  };

  /**
  * @class PackBucket
  * @brief Cards in a pool avoid listing duplicates by bundling them in a counted bucket 
  * 
  * Users can select up to all of the cards in a bucket. The bucket will remain in the list but at 0. 
  */
  class PoolBucket {
  private:
    unsigned size;
    unsigned maxSize;
    Battle::Card info;

  public:
    PoolBucket(unsigned size, Battle::Card info) : size(size), maxSize(size), info(info) { }
    ~PoolBucket() { }

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

private:
  std::vector<FolderSlot> folderCardSlots; /*!< Rows in the folder that can be inserted with cards or replaced */
  std::vector<PoolBucket> poolCardBuckets; /*!< Rows in the pack that represent how many of a card are left */
  bool hasFolderChanged; /*!< Flag if folder needs to be saved before quitting screen */
  Camera camera;
  CardFolder& folder;

  // Menu name font
  Font font;
  Text menuLabel;

  // Selection input delays
  double maxSelectInputCooldown; // half of a second
  double selectInputCooldown;

  // Card UI font
  Font cardFont;
  Text cardLabel;

  Font numberFont;
  Text numberLabel;

  // Card description font
  Font cardDescFont;
  Text cardDesc;

  // folder menu graphic
  sf::Sprite bg;
  sf::Sprite folderDock, packDock;
  sf::Sprite scrollbar;
  sf::Sprite cardHolder, packCardHolder;
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
    int maxCardsOnScreen{ 0 };
    int currCardIndex{ 0 };
    int lastCardOnScreen{ 0 }; // index
    int prevIndex{ -1 }; // for effect
    int numOfCards{ 0 };
    int swapCardIndex{ -1 }; // -1 for unselected, otherwise ID
  } folderView, packView;

  ViewMode currViewMode;
  ViewMode prevViewMode;

  double totalTimeElapsed;
  double frameElapsed;
 
  bool extendedHold{ false }; //!< If held for a 2nd pass, scroll quickly
  InputEvent lastKey{};
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

  void DrawFolder(sf::RenderTarget& surface);
  void DrawPool(sf::RenderTarget& surface);
  void ExcludeFolderDataFromPool();
  void PlaceFolderDataIntoCardSlots();
  void PlaceLibraryDataIntoBuckets();
  void WriteNewFolderData();

  template<typename ElementType>
  void RefreshCurrentCardDock(CardView& view, const std::vector<ElementType>& list);

public:
  std::string FormatCardDesc(const std::string&& desc);

  void onStart() override;
  void onUpdate(double elapsed) override;
  void onLeave() override;
  void onExit() override;
  void onEnter() override;
  void onResume() override;
  void onDraw(sf::RenderTexture& surface) override;
  void onEnd() override;

  FolderEditScene(swoosh::ActivityController&, CardFolder& folder);
  ~FolderEditScene();
};

template<typename ElementType>
void FolderEditScene::RefreshCurrentCardDock(FolderEditScene::CardView& view, const std::vector<ElementType>& list)
{
  if (view.currCardIndex < list.size()) {
    ElementType slot = list[view.currCardIndex]; // copy data, do not mutate it

    // If we have selected a new card, display the appropriate texture for its type
    if (view.currCardIndex != view.prevIndex) {
      sf::Sprite& sprite = currViewMode == ViewMode::folder ? cardHolder : packCardHolder;
      Battle::Card card;
      slot.GetCard(card); // Returns and frees the card from the bucket, this is why we needed a copy

      switch (card.GetClass()) {
      case Battle::CardClass::mega:
        sprite.setTexture(*LOAD_TEXTURE(FOLDER_CHIP_HOLDER_MEGA));
        break;
      case Battle::CardClass::giga:
        sprite.setTexture(*LOAD_TEXTURE(FOLDER_CHIP_HOLDER_GIGA));
        break;
      case Battle::CardClass::dark:
        sprite.setTexture(*LOAD_TEXTURE(FOLDER_CHIP_HOLDER_DARK));
        break;
      default:
        sprite.setTexture(*LOAD_TEXTURE(FOLDER_CHIP_HOLDER));
      }
    }
  }
}