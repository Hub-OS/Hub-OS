#include "bnPlayerCustScene.h"

constexpr size_t BAD_GRID_POS = std::numeric_limits<size_t>::max();
constexpr float GRID_START_X = 5.f;
constexpr float GRID_START_Y = 23.f;

enum class Blocks : uint8_t {
  White = 0,
  Red,
  Green,
  Blue,
  Pink,
  Yellow,
  Size
};

PlayerCustScene::Piece* generateRandomBlock() {
  PlayerCustScene::Piece* p = new PlayerCustScene::Piece();

  size_t x{};

  while (x == 0) {
    // generate smaller 3x3 shapes
    for (size_t i = 0; i < 3; i++) {
      for (size_t j = 0; j < 3; j++) {
        size_t index = (j * 5) + i;
        size_t k = rand() % 2;
        p->shape[index] = k;
        x += k;

        if (k) {
          p->maxWidth = std::max(i, p->maxWidth);
          p->maxHeight = std::max(j, p->maxHeight);
        }
      }
    }
  }

  p->typeIndex = rand() % static_cast<uint8_t>(Blocks::Size);
  p->specialType = rand() % 2;

  return p;
}

PlayerCustScene::PlayerCustScene(swoosh::ActivityController& controller):
  font(Font::Style::thin),
  text(font),
  textbox(sf::Vector2f(4, 255)),
  questionInterface(nullptr),
  Scene(controller)
{
  textbox.SetTextSpeed(1.0f);
  gotoNextScene = true;

  auto load_texture = [this](const std::string& path) {
    return Textures().LoadTextureFromFile(path);
  };

  bg = sf::Sprite(*load_texture("resources/scenes/cust/bg.png"));
  bg.setScale(2.f, 2.f);

  sceneLabel = sf::Sprite(*load_texture("resources/scenes/cust/scene_label.png"));
  sceneLabel.setPosition(sf::Vector2f(20.f, 8.0f));
  sceneLabel.setScale(2.f, 2.f);

  cursor = sf::Sprite(*load_texture("resources/scenes/cust/red_cursor.png"));
  cursor.setScale(2.f, 2.f);
  cursor.setTextureRect({ 0, 0, 16, 16 });

  claw = sf::Sprite(*load_texture("resources/scenes/cust/claw.png"));
  claw.setScale(2.f, 2.f);
  claw.setTextureRect({ 38, 0, 14, 14 });

  gridSprite = sf::Sprite(*load_texture("resources/scenes/cust/grid.png"));
  gridSprite.setTextureRect({ 0, 0, 127, 130 });
  gridSprite.setPosition((GRID_START_X*2.)+12, (GRID_START_Y*2.)-8);
  gridSprite.setScale(2.f, 2.f);
  
  for (uint8_t i = 0; i < static_cast<uint8_t>(Blocks::Size); i++) {
    blockInUseTable[i] = false;
    blockTextures.push_back(load_texture("resources/scenes/cust/cust_blocks_" + std::to_string(i) + ".png"));
  }

  setView(sf::Vector2u(480, 320));
}

PlayerCustScene::~PlayerCustScene()
{
  std::map<Piece*, bool> deleted;

  for (Piece* p : grid) {
    if (p && !deleted[p]) {
      deleted[p] = true;
      delete p;
    }
  }
}

bool PlayerCustScene::canPieceFit(Piece* piece, size_t loc)
{
  size_t x = loc % 7;
  size_t y = loc / 7;
  return (piece->maxWidth + x) < 7 && (piece->maxHeight + y) < 7;
}

bool PlayerCustScene::doesPieceOverlap(Piece* piece, size_t loc)
{
  size_t index = loc;
  for (size_t i = 0; i < 5; i++) {
    for (size_t j = 0; j < 5; j++) {
      if (piece->shape[(j * 5) + i]) {
        if (grid[index + j]) return true;
      }
    }
    index += 7; // next row
  }

  return false;
}

bool PlayerCustScene::insertPiece(Piece* piece, size_t loc)
{
  size_t shape_half = (piece->shape.size() / 2);

  if (loc < shape_half) return false;
  size_t start = loc - shape_half;
  size_t center = loc; // temp value

  if (!canPieceFit(piece, start) || doesPieceOverlap(piece, start)) {
    return false;
  }

  for (size_t i = 0; i < 5; i++) {
    for (size_t j = 0; j < 5; j++) {
      if (j == 3 && i == 2) {
        center = start +(i*7)+j;
      }

      if (piece->shape[(i * 5) + j]) {
        grid[start +(i*7) + j] = piece;
      }
    }
  }

  centerHash[piece] = center;
  return true;
}

void PlayerCustScene::removePiece(Piece* piece)
{
  size_t index = getPieceCenter(piece);
  if (index == BAD_GRID_POS) return;
  index = index - (piece->shape.size() / 2); // start scanning here

  bool scan = true;

  for (size_t i = 0; i < 5 && scan; i++) {
    for (size_t j = 0; j < 5 && scan; j++) {
      scan = (index + (i*7) + j) < grid.size();

      if (scan) {
        grid[index + (i*7) + j] = nullptr;
      }
    }
  }

  centerHash[piece] = BAD_GRID_POS;
}

size_t PlayerCustScene::getPieceCenter(Piece* piece)
{
  auto iter = centerHash.find(piece);
  if (iter != centerHash.end()) {
    return iter->second;
  }

  return BAD_GRID_POS;
}

void PlayerCustScene::drawPiece(sf::RenderTarget& surface, Piece* piece, const sf::Vector2f& cursorPos)
{
  sf::Sprite blockSprite;
  blockSprite.setScale(2.f, 2.f);

  for (size_t i = 0; i < 5; i++) {
    for (size_t j = 0; j < 5; j++) {
      size_t index = (i * 5) + j;
      if (piece->shape[index]) {
        float offsetX = -50.f + (i * 20.f * 2.f);
        float offsetY = -50.f + (j * 20.f * 2.f);
        blockSprite.setTexture(*blockTextures[piece->typeIndex], true);
        blockSprite.setTextureRect({ 40, 20, 20, 20 });
        blockSprite.setPosition({ cursorPos.x + offsetX, cursorPos.y + offsetY});
        surface.draw(blockSprite);
      }
    }
  }
}

void PlayerCustScene::onLeave()
{
}

void PlayerCustScene::onExit()
{
}

void PlayerCustScene::onEnter()
{
}

void PlayerCustScene::onResume()
{
}

void PlayerCustScene::onStart()
{
  gotoNextScene = false;
}

void PlayerCustScene::onUpdate(double elapsed)
{
  float cursorLocationX = (cursorLocation % 7) * 20.f * 2.f;
  float cursorLocationY = (cursorLocation / 7) * 20.f * 2.f;
  sf::Vector2f dest = { (GRID_START_X * 2)+cursorLocationX, (GRID_START_Y * 2)+cursorLocationY};
  sf::Vector2f cursorPos = cursor.getPosition();
  cursorPos.x = swoosh::ease::interpolate(0.5f, cursorPos.x, dest.x);
  cursorPos.y = swoosh::ease::interpolate(0.5f, cursorPos.y, dest.y);
  claw.setPosition(cursorPos);
  cursor.setPosition(cursorPos);

  if (!grabbedPiece && !insertingPiece) {
    if (Input().GetAnyKey() == sf::Keyboard::Space) {
      insertingPiece = generateRandomBlock();
      return;
    }

    if (Input().Has(InputEvents::pressed_interact)) {
      if (Piece* piece = grid[cursorLocation]) {
        removePiece(piece);
        grabbedPiece = piece;
        grabStartLocation = getPieceCenter(piece);
        return;
      }
    }
  }
  else if(insertingPiece) {
    if (Input().Has(InputEvents::pressed_cancel)) {
      delete insertingPiece;
      insertingPiece = nullptr;
      return;
    }

    if (Input().Has(InputEvents::pressed_confirm)) {
      if (insertPiece(insertingPiece, cursorLocation)) {
        insertingPiece = nullptr;
      }
      else {
        Audio().Play(AudioType::CHIP_ERROR);
      }
      return;
    }
  }
  else if (grabbedPiece) {
    if (Input().Has(InputEvents::pressed_cancel)) {
      insertPiece(grabbedPiece, grabStartLocation);
      grabbedPiece = nullptr;
      return;
    }

    if (Input().Has(InputEvents::pressed_confirm)) {
      if (insertPiece(grabbedPiece, cursorLocation)) {
        grabbedPiece = nullptr;
      }
      else {
        Audio().Play(AudioType::CHIP_ERROR);
      }
      return;
    }
  }

  if (Input().Has(InputEvents::pressed_move_left)) {
    if (cursorLocation % 7 > 0) {
      cursorLocation--;
    }
    return;
  } 

  if (Input().Has(InputEvents::pressed_move_right)) {
    if ((cursorLocation+1) % 7 > 0) {
      cursorLocation++;
    }
    return;
  }

  if (Input().Has(InputEvents::pressed_move_up)) {
    if (cursorLocation >= 7) {
      cursorLocation -= 7;
    }

    return;
  }

  if (Input().Has(InputEvents::pressed_move_down)) {
    if (cursorLocation + 7 < grid.size()) {
      cursorLocation += 7;
    }
    return;
  }
}

void PlayerCustScene::onDraw(sf::RenderTexture& surface)
{
  surface.draw(bg);
  surface.draw(sceneLabel);
  surface.draw(gridSprite);

  sf::Sprite blockSprite;
  blockSprite.setScale(2.f, 2.f);

  for (size_t i = 0; i < 7; i++) {
    for (size_t j = 0; j < 7; j++) {
      size_t index = (i * 7) + j;
      if (grid[index]) {
        float offsetX = (GRID_START_X * 2)+(i * 20.f * 2.f)-4.f;
        float offsetY = (GRID_START_Y * 2)+(j * 20.f * 2.f)-4.f;
        blockSprite.setTexture(*blockTextures[grid[index]->typeIndex], true);
        blockSprite.setTextureRect({ 20, 0, 20, 20 });
        blockSprite.setPosition({ offsetX, offsetY });
        surface.draw(blockSprite);
      }
    }
  }

  Piece* p = insertingPiece ? insertingPiece : grabbedPiece ? grabbedPiece : nullptr;
  if (p) {
    drawPiece(surface, p, claw.getPosition());
    surface.draw(claw);
  }
  else {
    surface.draw(cursor);
  }
}

void PlayerCustScene::onEnd()
{
}