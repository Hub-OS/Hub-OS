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
#include "bnChipFolder.h"
#include "bnAnimatedTextBox.h"

/**
 * @class FolderEditScene
 * @author mav
 * @date 04/05/19
 * @brief Edit folder contents and select from chip pool
 * @important the games, the chip pool chip count is not shared by folders
 * 
 * User can select chip and switch to the chip pool on the right side of the scene to select chips
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
  * @brief Chips in a pack avoid listing duplicates by bundling them in a counted bucket 
  * 
  * Users can select up to all of the chips in a bucket. The bucket will remain in the list but at 0. 
  */
  class PackBucket {
  private:
    unsigned size;
    unsigned maxSize;
    Chip info;

  public:
    PackBucket(unsigned size, Chip info) : size(size), maxSize(size), info(info) { }
    ~PackBucket() { }

    const bool IsEmpty() const { return size == 0; }
    const bool GetChip(Chip& copy) { if (IsEmpty()) return false; else copy = Chip(info); size--;  return true; }
    void AddChip() { size++; size = std::min(size, maxSize);  }
    const Chip& ViewChip() const { return info; }
    const unsigned GetCount() const { return size; }
  };

  /**
  * @class FolderSlot
  * @brief A selectable row in the folder to place new chips. When removing chips, an empty slot is left behind
  */
  class FolderSlot {
  private:
    bool occupied;
    Chip info;
  public:
    void AddChip(Chip other) {
      info = other;
      occupied = true;
    }

    const bool GetChip(Chip& copy) {
      if (!occupied) return false;

      copy = Chip(info);
      occupied = false;

      return true;
    }

    const bool IsEmpty() const {
      return !occupied;
    }

    const Chip& ViewChip() {
      return info;
    }
  };

  void ExcludeFolderDataFromPack();
  void PlaceFolderDataIntoChipSlots();
  void PlaceLibraryDataIntoBuckets();
  void WriteNewFolderData();

private:
  std::vector<FolderSlot> folderChipSlots; /*!< Rows in the folder that can be inserted with chips or replaced */
  std::vector<PackBucket> packChipBuckets; /*!< Rows in the pack that represent how many of a chip are left */
  bool hasFolderChanged; /*!< Flag if folder needs to be saved before quitting screen */
  Camera camera;
  ChipFolder& folder;
  AnimatedTextBox textbox;

  // Menu name font
  sf::Font* font;
  sf::Text* menuLabel;

  // Selection input delays
  double maxSelectInputCooldown; // half of a second
  double selectInputCooldown;

  // Chip UI font
  sf::Font *chipFont;
  sf::Text *chipLabel;

  sf::Font *numberFont;
  sf::Text *numberLabel;

  // Chip description font
  sf::Font *chipDescFont;
  sf::Text* chipDesc;

  // folder menu graphic
  sf::Sprite bg;
  sf::Sprite folderDock, packDock;
  sf::Sprite scrollbar;
  sf::Sprite stars;
  sf::Sprite chipHolder;
  sf::Sprite element;
  sf::Sprite folderCursor, folderSwapCursor;
  sf::Sprite packCursor, packSwapCursor;

  // Current chip graphic
  sf::Sprite chip;
  sf::IntRect cardSubFrame;

  sf::Sprite chipIcon;
  swoosh::Timer chipRevealTimer;
  swoosh::Timer easeInTimer;

  struct ChipView {
    int maxChipsOnScreen;
    int currChipIndex;
    int lastChipOnScreen; // index
    int prevIndex; // for effect
    int numOfChips;
    int swapChipIndex; // -1 for unselected, otherwise ID
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
  std::string FormatChipDesc(const std::string&& desc);

  virtual void onStart();
  virtual void onUpdate(double elapsed);
  virtual void onLeave();
  virtual void onExit();
  virtual void onEnter();
  virtual void onResume();
  virtual void onDraw(sf::RenderTexture& surface);
  virtual void onEnd();

  FolderEditScene(swoosh::ActivityController&, ChipFolder& folder);
  virtual ~FolderEditScene();
};
