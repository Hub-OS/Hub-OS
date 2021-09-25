#include "bnPlayerCustScene.h"

constexpr size_t BAD_GRID_POS = std::numeric_limits<size_t>::max();
constexpr size_t MAX_ALLOWED_TYPES = 4u;
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
  bool first = true;

  while (x == 0) {
    // generate smaller 3x3 shapes
    for (size_t i = 1; i < 4; i++) { // start one row down
      for (size_t j = 0; j < 3; j++) {
        size_t index = (i * p->BLOCK_SIZE) + j;
        size_t k = rand() % 2;
        p->shape[index] = k;
        x += k;

        if (first) {
          p->startX = j;
          p->startY = i;
        }

        if (k) {
          p->maxWidth = std::max(j, p->maxWidth);
          p->maxHeight = std::max(i, p->maxHeight);
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

  // Selection input delays
  maxSelectInputCooldown = 0.25; // 4th of a second
  selectInputCooldown = maxSelectInputCooldown;

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
    blockTypeInUseTable[i] = 0u;
    blockTextures.push_back(load_texture("resources/scenes/cust/cust_blocks_" + std::to_string(i) + ".png"));
  }

  sf::IntRect buttonRect = {0, 0, 73, 19};
  blueButtonSprite = sf::Sprite(*load_texture("resources/scenes/cust/item_name_container.png"));
  blueButtonSprite.setTextureRect(buttonRect);
  blueButtonSprite.setScale(2.f, 2.f);

  greenButtonSprite = sf::Sprite(*load_texture("resources/scenes/cust/run.png"));
  greenButtonSprite.setTextureRect(buttonRect);
  greenButtonSprite.setScale(2.f, 2.f);

  infoBox = sf::Sprite(*load_texture("resources/scenes/cust/info.png"));
  infoBox.setScale(2.f, 2.f);
  infoBox.setPosition({ 280, 188 });

  sf::Vector2f textPosition = infoBox.getPosition();
  textPosition.y += 20.f;
  textPosition.x += 8.f;
  text.setPosition(textPosition);
  text.setScale(2.f, 2.f);

  auto progressBarTexture = load_texture("resources/scenes/cust/progress_bar.png");
  progressBarTexture->setRepeated(true);

  progressBarUVs = { 0, 0, 118, 14 };
  progressBar = sf::Sprite(*progressBarTexture);
  progressBar.setScale(2.f, 2.f);
  progressBar.setPosition((GRID_START_X * 2.) + 20, 168);
  progressBar.setTextureRect(progressBarUVs); // source is 69x14 pixels
  progressBar.setColor(sf::Color(255, 255, 255, 200));

  // set screen view
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
  size_t x = loc % GRID_SIZE;
  size_t y = loc / GRID_SIZE;
  return (piece->maxWidth + x) < GRID_SIZE && (piece->maxHeight + y) < GRID_SIZE;
}

bool PlayerCustScene::doesPieceOverlap(Piece* piece, size_t loc)
{
  size_t index = loc;
  for (size_t i = 0; i < Piece::BLOCK_SIZE; i++) {
    for (size_t j = 0; j < Piece::BLOCK_SIZE; j++) {
      if (piece->shape[(i * Piece::BLOCK_SIZE) + j]) {
        if (grid[index + j]) return true;
      }
    }
    index += GRID_SIZE; // next row
  }

  return false;
}

bool PlayerCustScene::insertPiece(Piece* piece, size_t loc)
{
  size_t shape_half = ((Piece::BLOCK_SIZE / 2)*GRID_SIZE)+(GRID_SIZE-Piece::BLOCK_SIZE);

  if (loc < shape_half) return false;
  size_t start = loc - shape_half;
  size_t center = loc; // where should pickup the piece where it was inserted

  if (!canPieceFit(piece, start) || doesPieceOverlap(piece, start)) {
    return false;
  }

  // update grid pos
  loc = start;

  for (size_t i = 0; i < Piece::BLOCK_SIZE; i++) {
    for (size_t j = 0; j < Piece::BLOCK_SIZE; j++) {
      if (piece->shape[(i * Piece::BLOCK_SIZE) + j]) {
        grid[loc + j] = piece;
      }
    }
    loc += GRID_SIZE; // next row
  }

  centerHash[piece] = center;

  // update the block types in use table
  blockTypeInUseTable[piece->typeIndex]++;

  return true;
}

bool PlayerCustScene::isGridEdge(size_t y, size_t x)
{
  return x == 0 || x == GRID_SIZE-1u || y == GRID_SIZE-1u || y == 0;
}

void PlayerCustScene::removePiece(Piece* piece)
{
  size_t index = getPieceCenter(piece);
  if (index == BAD_GRID_POS) return;
  index = index - (((Piece::BLOCK_SIZE / 2)*GRID_SIZE)+(GRID_SIZE-Piece::BLOCK_SIZE)); // start scanning here

  bool scan = true;

  for (size_t i = 0; i < Piece::BLOCK_SIZE && scan; i++) {
    for (size_t j = 0; j < Piece::BLOCK_SIZE && scan; j++) {
      // crash prevention, shouldn't be necessary if everything is programmed correctly
      scan = (index + j) < grid.size();

      if (scan && piece->shape[(i*Piece::BLOCK_SIZE)+j]) {
        grid[index + j] = nullptr;
      }
    }
    std::cout << std::endl;
    index += GRID_SIZE;
  }

  // update the block types in use table
  blockTypeInUseTable[piece->typeIndex]--;

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

sf::Vector2f PlayerCustScene::gridCursorToScreen()
{
  float cursorLocationX = (cursorLocation % GRID_SIZE) * 20.f * 2.f;
  float cursorLocationY = (cursorLocation / GRID_SIZE) * 20.f * 2.f;
  return { (GRID_START_X * 2) + cursorLocationX, (GRID_START_Y * 2) + cursorLocationY };
}

void PlayerCustScene::drawPiece(sf::RenderTarget& surface, Piece* piece, const sf::Vector2f& cursorPos)
{
  sf::Sprite blockSprite;
  blockSprite.setScale(2.f, 2.f);

  for (size_t i = 0; i < Piece::BLOCK_SIZE; i++) {
    for (size_t j = 0; j < Piece::BLOCK_SIZE; j++) {
      size_t index = (i * Piece::BLOCK_SIZE) + j;
      if (piece->shape[index]) {
        float halfOffset = (((Piece::BLOCK_SIZE / 2)) * -20.f)*2.f;
        float offsetX = halfOffset + (j * 20.f * 2.f);
        float offsetY = halfOffset + (i * 20.f * 2.f);
        blockSprite.setTexture(*blockTextures[piece->typeIndex], true);
        blockSprite.setTextureRect({ 40, 20, 20, 20 });
        blockSprite.setPosition({ cursorPos.x + offsetX, cursorPos.y + offsetY});
        surface.draw(blockSprite);
      }
    }
  }
}

void PlayerCustScene::consolePrintGrid()
{
  for (size_t i = 0; i < GRID_SIZE; i++) {
    for (size_t j = 0; j < GRID_SIZE; j++) {
      if (grid[(i * GRID_SIZE) + j]) {
        std::cout << "X";
      }
      else {
        std::cout << ".";
      }
    }
    std::cout << std::endl;
  }
}

void PlayerCustScene::executeLeftKey()
{
  if (itemListSelected) {
    itemListSelected = false;
    return;
  }

  if (cursorLocation % GRID_SIZE > 0) {
    cursorLocation--;
  }
}

void PlayerCustScene::executeRightKey()
{
  if ((cursorLocation + 1) % GRID_SIZE > 0) {
    cursorLocation++;
  }
  else if(!grabbedPiece && !insertingPiece) {
    itemListSelected = true;
  }
}

void PlayerCustScene::executeUpKey()
{
  if (cursorLocation >= GRID_SIZE) {
    cursorLocation -= GRID_SIZE;
  }
}

void PlayerCustScene::executeDownKey()
{
  if (cursorLocation + GRID_SIZE < grid.size()) {
    cursorLocation += GRID_SIZE;
  }
}

void PlayerCustScene::handleInputDelay(double elapsed, void(PlayerCustScene::*executeFunc)())
{
  selectInputCooldown -= elapsed;

  if (selectInputCooldown <= 0) {
    if (!extendedHold) {
      selectInputCooldown = maxSelectInputCooldown;
      extendedHold = true;
    }
    else {
      selectInputCooldown = maxSelectInputCooldown * 0.25;
    }

    (this->*executeFunc)();
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
  textbox.Update(elapsed);
  progressBarUVs.left -= 1;
  progressBar.setTextureRect(progressBarUVs);

  sf::Vector2f dest = gridCursorToScreen();
  sf::Vector2f cursorPos = cursor.getPosition();
  cursorPos.x = swoosh::ease::interpolate(0.5f, cursorPos.x, dest.x);
  cursorPos.y = swoosh::ease::interpolate(0.5f, cursorPos.y, dest.y);
  claw.setPosition(cursorPos);
  cursor.setPosition(cursorPos);

  if (questionInterface) {
    if (!textbox.IsOpen()) return;

    if (Input().Has(InputEvents::pressed_ui_left)) {
      questionInterface->SelectYes();
      return;
    }

    if (Input().Has(InputEvents::pressed_ui_right)) {
      questionInterface->SelectNo();
      return;
    }

    if (Input().Has(InputEvents::pressed_confirm)) {
      questionInterface->ConfirmSelection();
      return;
    }
  }

  if (itemListSelected) {
    if (Input().Has(InputEvents::pressed_ui_left)) {
      executeLeftKey();
      return;
    }

    if (!Input().Has(InputEvents::pressed_confirm)) return; // TODO: for now

    auto onYes = [this]() {
      questionInterface = nullptr;
      textbox.Close();
    };

    auto onNo = [this]() {
      questionInterface = nullptr;
      textbox.Close();
    };

    questionInterface = new Question("Run the program?", onYes, onNo);
    textbox.EnqueMessage(questionInterface);
    textbox.Open();
    return;
  }

  if (!grabbedPiece && !insertingPiece) {
    if (Input().GetAnyKey() == sf::Keyboard::Space) {
      insertingPiece = generateRandomBlock();
      return;
    }

    if (Input().Has(InputEvents::pressed_confirm)) {
      if (Piece* piece = grid[cursorLocation]) {
        // interpolate to the center of the block
        grabStartLocation = getPieceCenter(piece);
        cursorLocation = std::min(grabStartLocation, grid.size()-1u);

        removePiece(piece);
        grabbedPiece = piece;
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
        Audio().Play(AudioType::CHIP_ERROR, AudioPriority::high);
      }
      return;
    }

    if (Input().Has(InputEvents::pressed_shoulder_left)) {
      insertingPiece->rotateLeft();
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
        Audio().Play(AudioType::CHIP_ERROR, AudioPriority::high);
      }
      return;
    }

    if (Input().Has(InputEvents::pressed_shoulder_left)) {
      grabbedPiece->rotateLeft();
      return;
    }
  }

  if (Input().Has(InputEvents::pressed_ui_left) || Input().Has(InputEvents::held_ui_left)) {
    handleInputDelay(elapsed, &PlayerCustScene::executeLeftKey);
    return;
  } 

  if (Input().Has(InputEvents::pressed_ui_right) || Input().Has(InputEvents::held_ui_right)) {
    handleInputDelay(elapsed, &PlayerCustScene::executeRightKey);
    return;
  }

  if (Input().Has(InputEvents::pressed_ui_up) || Input().Has(InputEvents::held_ui_up)) {
    handleInputDelay(elapsed, &PlayerCustScene::executeUpKey);
    return;
  }

  if (Input().Has(InputEvents::pressed_ui_down) || Input().Has(InputEvents::held_ui_down)) {
    handleInputDelay(elapsed, &PlayerCustScene::executeDownKey);
    return;
  }

  //else
  selectInputCooldown = 0;
  extendedHold = false;
}

void PlayerCustScene::onDraw(sf::RenderTexture& surface)
{
  surface.draw(bg);
  surface.draw(sceneLabel);
  surface.draw(gridSprite);

  sf::Sprite blockSprite;
  blockSprite.setScale(2.f, 2.f);

  size_t count{};
  for (auto& [key, value] : blockTypeInUseTable) {
    if (value == 0) continue;
    float x = (gridSprite.getPosition().x + (8.f*2.f)) + (count*(14.+1.)*2.f);
    float y = gridSprite.getPosition().y + 2.f*2.f;
    blockSprite.setTexture(*blockTextures[key], true);
    blockSprite.setTextureRect({ 60, 0, 14, 9 });
    blockSprite.setPosition({ x, y });
    surface.draw(blockSprite);

    if (count++ == MAX_ALLOWED_TYPES) break;
  }

  for (size_t i = 0; i < GRID_SIZE; i++) {
    for (size_t j = 0; j < GRID_SIZE; j++) {
      size_t index = (i * GRID_SIZE) + j;
      if (grid[index]) {
        float offsetX = (GRID_START_X * 2)+(j * 20.f * 2.f)-4.f;
        float offsetY = (GRID_START_Y * 2)+(i * 20.f * 2.f)-4.f;
        blockSprite.setTexture(*blockTextures[grid[index]->typeIndex], true);
        blockSprite.setTextureRect({ 20, 0, 20, 20 });
        blockSprite.setPosition({ offsetX, offsetY });

        if (isGridEdge(i, j)) {
          sf::Color color = sf::Color::White;
          color.a = 100;
          blockSprite.setColor(color);
        }
        else {
          blockSprite.setColor(sf::Color::White);
        }
        surface.draw(blockSprite);
      }
    }
  }

  // draw items
  sf::Vector2f bottom;
  bottom.x = 140.f;

  for (size_t i = 0; i < pieces.size(); i++) {
    bottom.y += blueButtonSprite.getLocalBounds().height + 20.f;
  }

  if (bottom.y == 0) {
    bottom.y = blueButtonSprite.getLocalBounds().height + 20.f;
  }

  // draw run button
  greenButtonSprite.setPosition(bottom.x * greenButtonSprite.getScale().x, bottom.y*greenButtonSprite.getScale().y);
  surface.draw(greenButtonSprite);

  // draw info box
  surface.draw(infoBox);

  if (itemListSelected) {
    sf::Vector2f pos = text.getPosition();
    text.SetString("RUN?");
    text.SetColor(sf::Color(33, 115, 140));
    text.setPosition(pos.x+2.f, pos.y+2.f);
    surface.draw(text);
    text.SetColor(sf::Color::White);
    text.setPosition(pos);
    surface.draw(text);

    // TODO: for now
    surface.draw(progressBar);
  }
  else {
    // claw/cursor always draws on top
    Piece* p = insertingPiece ? insertingPiece : grabbedPiece ? grabbedPiece : nullptr;
    if (p) {
      drawPiece(surface, p, gridCursorToScreen());
      surface.draw(claw);
    }
    else {
      surface.draw(cursor);
    }
  }

  // textbox is top over everything
  surface.draw(textbox);
}

void PlayerCustScene::onEnd()
{
}