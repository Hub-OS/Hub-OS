#pragma once
#include <Swoosh/Ease.h>

#include "bnScene.h"
#include "bnInputManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnGame.h"
#include "bnAnimation.h"
#include "bnLanBackground.h"
#include "bnAnimatedTextBox.h"
#include "bnMessageQuestion.h"

class PlayerCustScene : public Scene {
public:
  struct Piece {
    std::string name = "Unnamed";
    std::string description = "N/A";
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
    }

    void rotateRight() {
      rotateLeft();
      rotateLeft();
      rotateLeft();
    }

    void commit() {
      finalRot = rotates % 4;
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
  };
private:
  static constexpr size_t GRID_SIZE = 7u;

  enum class state : char {
    usermode = 0,
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
  double scaffolding{1.f};
  sf::Sprite bg;
  sf::Sprite cursor, itemArrowCursor;
  sf::Sprite claw;
  sf::Sprite sceneLabel;
  sf::Sprite gridSprite;
  sf::Sprite greenButtonSprite;
  sf::Sprite blueButtonSprite;
  sf::Sprite infoBox, previewBox;
  sf::Sprite track;
  sf::Sprite progressBar;
  sf::IntRect progressBarUVs;
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
  Animation gridAnim, cursorAnim, clawAnim, blockAnim, buttonAnim, trackAnim;
  AnimatedTextBox textbox;
  Question* questionInterface{ nullptr };

  int maxItemsOnScreen{}; /*!< The number of items to display max in box area */
  int currItemIndex{}; /*!< Current index in list */
  int numOfItems{}; /*!< Number of index in list */

  bool gotoNextScene{}; /*!< If true, user cannot interact */
  bool itemListSelected{}; // If the item list if not selected, it implies the grid area is

  void startCompile();
  bool isCompileFinished();

  void removePiece(Piece* piece);
  bool hasLeftInput();
  bool hasRightInput();
  bool hasUpInput();
  bool hasDownInput();
  bool canPieceFit(Piece* piece, size_t loc);
  bool doesPieceOverlap(Piece* piece, size_t loc);
  bool insertPiece(Piece* piece, size_t loc);
  bool isGridEdge(size_t y, size_t x);
  bool handleSelectItemFromList();
  bool handleArrowKeys(double elapsed);
  bool handlePieceAction(Piece*& piece, void(PlayerCustScene::* executeFunc)());
  size_t getPieceCenter(Piece* piece);
  sf::Vector2f gridCursorToScreen();
  void drawPiece(sf::RenderTarget& surface, Piece* piece, const sf::Vector2f& pos);
  void drawPreview(sf::RenderTarget& surface, Piece* piece, const sf::Vector2f& pos);
  void consolePrintGrid();
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
  void handleInputDelay(double elapsed, void(PlayerCustScene::* executeFunc)());
  void selectGridUI();
  void selectItemUI(size_t idx);
  void quitScene();
public:

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
  PlayerCustScene(swoosh::ActivityController&, std::vector<Piece*> pieces);

  /**
   * @brief deconstructor
   */
  ~PlayerCustScene();
};
