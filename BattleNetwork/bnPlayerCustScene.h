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
    size_t typeIndex{};
    size_t maxWidth{}, maxHeight{};
    bool specialType{};
    std::array<uint8_t, 25> shape{}; // 5x5
  };
private:

  // Menu name font
  Font font; /*!< Font of the  menu name label*/
  Text text;

  // Selection input delays
  bool extendedHold{ false }; /*!< 2nd delay pass makes scrolling quicker */
  double maxSelectInputCooldown{}; /*!< Set to fraction of a second */
  double selectInputCooldown{}; /*!< The delay between reading user input */

  // folder menu graphics
  sf::Sprite bg;
  sf::Sprite cursor;
  sf::Sprite claw;
  sf::Sprite sceneLabel;
  sf::Sprite gridSprite;
  std::vector<std::shared_ptr<sf::Texture>> blockTextures;
  std::map<size_t, bool> blockInUseTable;

  std::map<Piece*, size_t> centerHash;
  std::array<Piece*, 49> grid{ nullptr }; // 7x7
  Piece* grabbedPiece{ nullptr }; // when moving from the grid to another location
  Piece* insertingPiece{ nullptr }; // when moving from the list to the grid
  size_t cursorLocation{}; // in grid-space
  size_t grabStartLocation{}; // in grid-space
  AnimatedTextBox textbox;
  Question* questionInterface{ nullptr };

  int maxItemsOnScreen{}; /*!< The number of items to display max in box area */
  int currItemIndex{}; /*!< Current index in list */
  int numOfItems{}; /*!< Number of index in list */

  double frameElapsed{};

  bool gotoNextScene{}; /*!< If true, user cannot interact */

  void removePiece(Piece* piece);
  bool canPieceFit(Piece* piece, size_t loc);
  bool doesPieceOverlap(Piece* piece, size_t loc);
  bool insertPiece(Piece* piece, size_t loc);
  size_t getPieceCenter(Piece* piece);
  void drawPiece(sf::RenderTarget& surface, Piece* piece, const sf::Vector2f& cursorPos);
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
  PlayerCustScene(swoosh::ActivityController&);

  /**
   * @brief deconstructor
   */
  ~PlayerCustScene();
};
