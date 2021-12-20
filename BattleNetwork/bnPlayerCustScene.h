#pragma once
#include <Swoosh/Ease.h>

#include "bnScene.h"
#include "bnInputManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnAnimation.h"
#include "bnLanBackground.h"
#include "bnAnimatedTextBox.h"
#include "bnMessageQuestion.h"
#include "bnPackageAddress.h"

class PlayerCustScene : public Scene {
public:
  struct Piece {
    enum class Types : uint8_t {
      white = 0,
      red,
      green,
      blue,
      pink,
      yellow,
      size
    };

    std::string name = "Unnamed";
    std::string description = "N/A";
    std::string uuid;
    static constexpr size_t BLOCK_SIZE = 5u;
    size_t typeIndex{};
    size_t maxWidth{}, maxHeight{};
    size_t startX{}, startY{}; // index where the first entry for the shape is
    size_t rotates{}, finalRot{}; // how many times rotated
    bool specialType{};
    std::array<uint8_t, BLOCK_SIZE* BLOCK_SIZE> shape{}; // 5x5
    
    void rotateLeft() {
      auto newShape = shape;
      size_t idx = 0u;
      for (size_t i = 0; i < BLOCK_SIZE; i++) {
        for (size_t j = BLOCK_SIZE; j > 0u; j--) {
          newShape[idx++] = shape[((j - 1u) * BLOCK_SIZE) + i];
        }
      }

      std::swap(newShape, shape);
      rotates++;

      calculateDimensions(); // TODO: this can be embedded in the loop above for optimization
    }

    void rotateRight() {
      rotateLeft();
      rotateLeft();
      rotateLeft();
    }

    void commit() {
      finalRot = (finalRot + rotates) % 4;
      rotates = 0;
    }

    void revert() {
      size_t count = rotates % 4;

      if (count == 0) return;

      for (size_t i = 0; i < 4u-count; i++) {
        rotateLeft();
      }

      commit();
    }

    void calculateDimensions() {
      bool first = true;
      size_t min_i{}, min_j{}, max_i{}, max_j{};

      for (size_t i = 0; i < BLOCK_SIZE; i++) {
        for (size_t j = 0; j < BLOCK_SIZE; j++) {
          size_t index = (i * BLOCK_SIZE) + j;
          uint8_t k = shape[index];

          if (k) {
            if (!first) {
              // max of rect
              max_j = std::max(j, max_j);
              max_i = std::max(i, max_i);

              // min of rect
              min_j = std::min(j, min_j);
              min_i = std::min(i, min_i);
            }
            else {
              max_j = min_j = j;
              max_i = min_i = i;
              first = false;
            }
          }
        }
      }

      startX = min_j;
      startY = min_i;
      maxWidth = (max_j - min_j) + 1u;
      maxHeight = (max_i - min_i) + 1u;
    }
  };
private:
  static constexpr size_t GRID_SIZE = 7u;

  enum class state : char {
    usermode = 0,
    block_prompt,
    compiling,
    waiting,
    finishing
  } state{};

  // Menu name font
  Text infoText, itemText, hoverText;

  // Selection input delays
  bool extendedHold{ false }; /*!< 2nd delay pass makes scrolling quicker */
  double maxSelectInputCooldown{}; /*!< Set to fraction of a second */
  double selectInputCooldown{}; /*!< The delay between reading user input */

  // progress bar
  double progress{}, maxProgressTime{ 3. };

  // scene graphics
  double blockFlashElapsed{}, buttonFlashElapsed{};
  double frameElapsed{};
  float scaffolding{1.f};
  sf::Sprite bg;
  sf::Sprite cursor, itemArrowCursor;
  sf::Sprite claw;
  sf::Sprite sceneLabel;
  sf::Sprite gridSprite;
  sf::Sprite greenButtonSprite;
  sf::Sprite blueButtonSprite;
  sf::Sprite infoBox, previewBox, menuBox;
  sf::Sprite blockShadowVertical, blockShadowHorizontal;
  sf::Sprite track;
  sf::Sprite progressBar;
  sf::IntRect progressBarUVs;
  std::string playerUUID;
  std::shared_ptr<sf::Texture> cursorTexture, miniblocksTexture;
  std::vector<std::shared_ptr<sf::Texture>> blockTextures;
  std::shared_ptr<sf::SoundBuffer> compile_start, compile_complete, compile_no_item, compile_item;
  std::vector<Piece*> pieces;
  std::map<Piece*, size_t> centerHash;
  std::map<Piece*, bool> compiledHash;
  std::map<size_t, size_t> blockTypeInUseTable;
  std::array<Piece*, GRID_SIZE* GRID_SIZE> grid{ nullptr }; // 7x7
  Piece* grabbingPiece{ nullptr }; // when moving from the grid to another location
  Piece* insertingPiece{ nullptr }; // when moving from the list to the grid
  size_t cursorLocation{}; // in grid-space
  size_t grabStartLocation{}; // in grid-space
  size_t listStart{};
  size_t currCompileIndex{};
  Animation gridAnim, cursorAnim, clawAnim, blockAnim, buttonAnim, trackAnim, blockShadowVertAnim, blockShadowHorizAnim, menuAnim;
  AnimatedTextBox textbox;
  Question* questionInterface{ nullptr };

  int maxItemsOnScreen{}; /*!< The number of items to display max in box area */
  int currItemIndex{}; /*!< Current index in list */
  int numOfItems{}; /*!< Number of index in list */

  bool gotoNextScene{}; /*!< If true, user cannot interact */
  bool itemListSelected{}; // If the item list if not selected, it implies the grid area is

  bool isCompileFinished();
  bool hasLeftInput();
  bool hasRightInput();
  bool hasUpInput();
  bool hasDownInput();
  bool isBlockValid(Piece* piece);
  bool canPieceFit(Piece* piece, size_t loc);
  bool doesPieceOverlap(Piece* piece, size_t loc);
  bool insertPiece(Piece* piece, size_t loc);
  bool isGridEdge(size_t y, size_t x);
  bool handleSelectItemFromList();
  bool handleUIKeys(double elapsed);
  void handleMenuUIKeys(double elapsed);
  void handleGrabAction();
  bool handlePieceAction(Piece*& piece, void(PlayerCustScene::* executeFunc)());
  size_t getPieceCenter(Piece* piece);
  size_t getPieceStart(Piece* piece, size_t center);
  sf::Vector2f gridCursorToScreen();
  sf::Vector2f blockToScreen(size_t y, size_t x);
  void loadFromSave();
  void completeAndSave();
  void drawEdgeBlock(sf::RenderTarget& surface, Piece* piece, size_t y, size_t x);
  void drawPiece(sf::RenderTarget& surface, Piece* piece, const sf::Vector2f& pos);
  void drawPreview(sf::RenderTarget& surface, Piece* piece, const sf::Vector2f& pos);
  void removePiece(Piece* piece);
  void consolePrintGrid();
  void startCompile();
  void startScaffolding();
  void animateButton(double elapsed);
  void animateCursor(double elapsed);
  void animateScaffolding(double elapsed);
  void animateGrid();
  void animateBlock(double elapsed, Piece* p = nullptr);
  void refreshBlock(Piece* p, sf::Sprite& sprite);
  void refreshButton(size_t idx);
  void refreshTrack();
  void executeLeftKey();
  void executeRightKey();
  void executeUpKey();
  void executeDownKey();
  void executeCancelInsert();
  void executeCancelGrab();
  void updateCursorHoverInfo();
  void updateItemListHoverInfo();
  void updateMenuPosition();
  void handleInputDelay(double elapsed, void(PlayerCustScene::* executeFunc)());
  void selectGridUI();
  void selectItemUI(size_t idx);
  void quitScene();
public:

  static std::vector<PackageAddress> getInstalledBlocks(const std::string& playerID, const GameSession& session);

  void onLeave() override;
  void onExit() override;
  void onEnter() override;
  void onResume() override;
  void onStart() override;

  /**
  * @brief Responds to user input and menu states
  * @param elapsed in seconds
  */
  void onUpdate(double elapsed) override;

  /**
   * @brief Interpolate folder positions and draws
   * @param surface
   */
  void onDraw(sf::RenderTexture& surface) override;

  /**
   * @brief Deletes all resources
   */
  void onEnd() override;

  /**
   * @brief
   *
   * Loads and initializes all default graphics and state values
   */
  PlayerCustScene(swoosh::ActivityController&, const std::string& playerUUID, std::vector<Piece*> pieces);

  /**
   * @brief deconstructor
   */
  ~PlayerCustScene();
};
