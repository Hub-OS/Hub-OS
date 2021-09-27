#include "bnPlayerCustScene.h"
#include "stx/string.h"

constexpr size_t BAD_GRID_POS = std::numeric_limits<size_t>::max();
constexpr size_t MAX_ALLOWED_TYPES = 4u;
constexpr size_t MAX_ITEMS_ON_SCREEN = 4u; // includes RUN button
constexpr float GRID_START_X = 4.f;
constexpr float GRID_START_Y = 23.f;
constexpr sf::Uint8 PROGRESS_MAX_ALPHA = 200;

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
      for (size_t j = 1; j < 4; j++) { // start one column over
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
  p->name = stx::rand_alphanum(8);
  p->description = stx::rand_alphanum(30);

  p->description[9] = '\n';
  p->description[19] = '\n';
  return p;
}

PlayerCustScene::PlayerCustScene(swoosh::ActivityController& controller):
  infoText(Font::Style::thin),
  itemText(Font::Style::thick),
  hoverText(Font::Style::thick),
  textbox(sf::Vector2f(4, 255)),
  questionInterface(nullptr),
  Scene(controller)
{
  textbox.SetTextSpeed(1.0f);
  gotoNextScene = true;

  // Selection input delays
  maxSelectInputCooldown = 0.25; // 4th of a second
  selectInputCooldown = maxSelectInputCooldown;

  auto load_audio = [this](const std::string& path) {
    return Audio().LoadFromFile(path);
  };

  compile_start = load_audio("resources/sfx/compile_start.ogg");
  compile_complete = load_audio("resources/sfx/compile_complete.ogg");
  compile_no_item = load_audio("resources/sfx/compile_empty_item.ogg");
  compile_item = load_audio("resources/sfx/compile_item.ogg");

  auto load_texture = [this](const std::string& path) {
    return Textures().LoadTextureFromFile(path);
  };

  cursorTexture = load_texture("resources/ui/textbox_cursor.png");
  itemArrowCursor = sf::Sprite(*cursorTexture);

  sf::FloatRect bounds = itemArrowCursor.getLocalBounds();
  itemArrowCursor.setScale(2.f, 2.f);
  itemArrowCursor.setOrigin({ bounds.width, 0. });

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

  previewBox = sf::Sprite(*load_texture("resources/scenes/cust/mini_block_container.png"));
  previewBox.setScale(2.f, 2.f);

  miniblocksTexture = load_texture("resources/scenes/cust/mini_blocks.png");

  // info text
  sf::Vector2f textPosition = infoBox.getPosition();
  textPosition.y += 20.f;
  textPosition.x += 8.f;
  infoText.setPosition(textPosition);
  infoText.setScale(2.f, 2.f);

  // item text
  itemText.setScale(2.f, 2.f);

  // hover text copies item text
  hoverText = itemText;

  // progress bar data
  auto progressBarTexture = load_texture("resources/scenes/cust/progress_bar.png");
  progressBarTexture->setRepeated(true);

  progressBarUVs = { 0, 0, 118, 14 };
  progressBar = sf::Sprite(*progressBarTexture);
  progressBar.setScale(2.f, 2.f);
  progressBar.setPosition((GRID_START_X * 2.) + 20, 168);
  progressBar.setTextureRect(progressBarUVs); // source is 69x14 pixels
  progressBar.setColor(sf::Color(255, 255, 255, PROGRESS_MAX_ALPHA));

  // set screen view
  setView(sf::Vector2u(480, 320));
}

PlayerCustScene::~PlayerCustScene()
{
  for (Piece* p : pieces) {
      delete p;
  }

  pieces.clear();
}

bool PlayerCustScene::canPieceFit(Piece* piece, size_t loc)
{
  size_t x = loc % GRID_SIZE;
  size_t y = loc / GRID_SIZE;

  // prevent inserting edges
  bool tl = (x == 0 && y == 0);
  bool tr = (x + piece->maxWidth + 1u == GRID_SIZE && y == 0);
  bool bl = (x == 0 && y + 1u == GRID_SIZE);
  bool br = (x + piece->maxWidth + 1u == GRID_SIZE && y + 1u == GRID_SIZE);

  if (tl || tr || bl || br) return false;

  return (piece->maxWidth + x) < GRID_SIZE && (piece->maxHeight + y) < GRID_SIZE;
}

bool PlayerCustScene::doesPieceOverlap(Piece* piece, size_t loc)
{
  size_t index = loc;
  for (size_t i = piece->startY; i <= piece->maxHeight; i++) {
    for (size_t j = piece->startX; j <= piece->maxWidth; j++) {
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

void PlayerCustScene::startCompile()
{
  infoText.SetString("RUNNING...");
  progress = 0;
  isCompiling = true;
  sf::Color color = sf::Color(255, 255, 255, PROGRESS_MAX_ALPHA);
  progressBarUVs.width = 0;
  progressBar.setTextureRect(progressBarUVs);
  progressBar.setColor(color);
  Audio().Play(compile_start);
}

bool PlayerCustScene::isCompileFinished()
{
  return progress >= maxProgressTime; // in seconds
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
    index += GRID_SIZE;
  }

  // update the block types in use table
  blockTypeInUseTable[piece->typeIndex]--;

  centerHash[piece] = BAD_GRID_POS;
}

bool PlayerCustScene::hasLeftInput()
{
  return Input().Has(InputEvents::pressed_ui_left) || Input().Has(InputEvents::held_ui_left);
}

bool PlayerCustScene::hasRightInput()
{
  return Input().Has(InputEvents::pressed_ui_right) || Input().Has(InputEvents::held_ui_right);
}

bool PlayerCustScene::hasUpInput()
{
  return Input().Has(InputEvents::pressed_ui_up) || Input().Has(InputEvents::held_ui_up);
}

bool PlayerCustScene::hasDownInput()
{
  return Input().Has(InputEvents::pressed_ui_down) || Input().Has(InputEvents::held_ui_down);
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

void PlayerCustScene::drawPiece(sf::RenderTarget& surface, Piece* piece, const sf::Vector2f& pos)
{
  sf::Sprite blockSprite;
  blockSprite.setScale(2.f, 2.f);

  sf::Color color = blockSprite.getColor();
  color.a = 140;
  blockSprite.setColor(color);

  for (size_t i = 0; i < Piece::BLOCK_SIZE; i++) {
    for (size_t j = 0; j < Piece::BLOCK_SIZE; j++) {
      size_t index = (i * Piece::BLOCK_SIZE) + j;
      if (piece->shape[index]) {
        float halfOffset = (((Piece::BLOCK_SIZE / 2)) * -20.f)*2.f;
        float offsetX = halfOffset + (j * 20.f * 2.f)-4.f;
        float offsetY = halfOffset + (i * 20.f * 2.f)-4.f;
        blockSprite.setTexture(*blockTextures[piece->typeIndex], true);
        blockSprite.setTextureRect({ 40, 20, 20, 20 });
        blockSprite.setPosition({ pos.x + offsetX, pos.y + offsetY});
        surface.draw(blockSprite);
      }
    }
  }
}

void PlayerCustScene::drawPreview(sf::RenderTarget& surface, Piece* piece, const sf::Vector2f& pos)
{
  sf::Sprite blockSprite;
  blockSprite.setScale(2.f, 2.f);

  previewBox.setPosition(pos);
  surface.draw(previewBox);

  for (size_t i = 0; i < Piece::BLOCK_SIZE; i++) {
    for (size_t j = 0; j < Piece::BLOCK_SIZE; j++) {
      size_t index = (i * Piece::BLOCK_SIZE) + j;
      if (piece->shape[index]) {
        float offsetX = (j * 3.f * 2.f) + 4.f;
        float offsetY = (i * 3.f * 2.f) + 4.f;
        blockSprite.setTexture(*miniblocksTexture, true);
        blockSprite.setTextureRect({ static_cast<int>(piece->typeIndex*3), 0, 3, 3 });
        blockSprite.setPosition({ pos.x + offsetX, pos.y + offsetY });
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

bool PlayerCustScene::handleSelectItemFromList()
{
  if (listStart >= pieces.size()) return false;

  auto iter = std::next(pieces.begin(), listStart);
  insertingPiece = *iter;
  pieces.erase(iter);

  // move cursor to the center of the grid
  cursorLocation = ((GRID_SIZE / 2) * GRID_SIZE) + (GRID_SIZE/2);
  itemListSelected = false;

  return true;
}

void PlayerCustScene::executeLeftKey()
{
  if (itemListSelected) {
    itemListSelected = false;
    Audio().Play(AudioType::CHIP_DESC_CLOSE, AudioPriority::low);
    return;
  }

  if (cursorLocation % GRID_SIZE > 0) {
    cursorLocation--;
    updateCursorHoverInfo();
    Audio().Play(AudioType::CHIP_SELECT, AudioPriority::low);
  }
}

void PlayerCustScene::executeRightKey()
{
  if ((cursorLocation + 1) % GRID_SIZE > 0) {
    cursorLocation++;
    updateCursorHoverInfo();
    Audio().Play(AudioType::CHIP_SELECT, AudioPriority::low);
  }
  else if(!grabbedPiece && !insertingPiece) {
    if (itemListSelected) return;
    itemListSelected = true;
    updateItemListHoverInfo();
    Audio().Play(AudioType::CHIP_DESC, AudioPriority::low);
  }
}

void PlayerCustScene::executeUpKey()
{
  if (itemListSelected) {
    if (listStart > 0) {
      listStart--;
      updateItemListHoverInfo();
      Audio().Play(AudioType::CHIP_SELECT, AudioPriority::low);
    }
    return;
  }

  if (cursorLocation >= GRID_SIZE) {
    cursorLocation -= GRID_SIZE;
    updateCursorHoverInfo();
    Audio().Play(AudioType::CHIP_SELECT, AudioPriority::low);
  }
}

void PlayerCustScene::executeDownKey()
{
  if (itemListSelected) {
    // pieces end iterator doubles as the `run` action item
    if (listStart+1u <= pieces.size()) {
      listStart++;
      updateItemListHoverInfo();
      Audio().Play(AudioType::CHIP_SELECT, AudioPriority::low);
    }
    return;
  }

  if (cursorLocation + GRID_SIZE < grid.size()) {
    cursorLocation += GRID_SIZE;
    updateCursorHoverInfo();
    Audio().Play(AudioType::CHIP_SELECT, AudioPriority::low);
  }
}

bool PlayerCustScene::handleArrowKeys(double elapsed)
{
  if (hasLeftInput()) {
    handleInputDelay(elapsed, &PlayerCustScene::executeLeftKey);
    return true;
  }

  if (hasRightInput()) {
    handleInputDelay(elapsed, &PlayerCustScene::executeRightKey);
    return true;
  }

  if (hasUpInput()) {
    handleInputDelay(elapsed, &PlayerCustScene::executeUpKey);
    return true;
  }

  if (hasDownInput()) {
    handleInputDelay(elapsed, &PlayerCustScene::executeDownKey);
    return true;
  }

  return false;
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

void PlayerCustScene::updateCursorHoverInfo()
{
  if (Piece* p = grid[cursorLocation]) {
    infoText.SetString(p->description);
    hoverText.SetString(p->name);

    sf::Vector2f pos = gridCursorToScreen();
    pos.y += 20.f * 2.f; // below the block it is on
    hoverText.setPosition(pos);

    sf::FloatRect bounds = hoverText.GetLocalBounds();

    float x = (cursorLocation % GRID_SIZE)/static_cast<float>(GRID_SIZE) * bounds.width;
    float y = (cursorLocation / GRID_SIZE) + 1u == GRID_SIZE ? 20.f*2.f : 0.f;
    hoverText.setOrigin({ x, y});
  }
  else {
    infoText.SetString("");
    hoverText.SetString("");
  }
}

void PlayerCustScene::updateItemListHoverInfo()
{
  if (listStart < pieces.size()) {
    infoText.SetString(pieces[listStart]->description);
  }
  else {
    infoText.SetString("RUN?");
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
  progressBarUVs.left -= isCompileFinished()?1:2;
  progressBarUVs.width = static_cast<int>(std::min(118., (118.*(progress/maxProgressTime))));
  progressBar.setTextureRect(progressBarUVs);

  sf::Vector2f dest = gridCursorToScreen();
  sf::Vector2f cursorPos = cursor.getPosition();
  cursorPos.x = swoosh::ease::interpolate(0.5f, cursorPos.x, dest.x);
  cursorPos.y = swoosh::ease::interpolate(0.5f, cursorPos.y, dest.y);
  claw.setPosition(cursorPos);
  cursor.setPosition(cursorPos);

  if (isCompiling) {
    progress += elapsed;
    if (isCompileFinished()) {
      isCompiling = false;
      infoText.SetString("OK");
      Audio().Play(compile_complete);
    }
    else {
      double percent = (progress / static_cast<double>(maxProgressTime));
      double block_time = static_cast<double>(maxProgressTime/ GRID_SIZE);

      // complete the text when we're 75% through the block
      std::string label = "None";
      double COMPLETE_AT_PROGRESS = .75;
      double block_progress = std::fmod(progress, block_time) / block_time;
      double text_progress = std::min(block_progress * 1. / COMPLETE_AT_PROGRESS, 1.);

      size_t loc = static_cast<size_t>(percent * GRID_SIZE);
      Piece* p = grid[(3u * GRID_SIZE) + loc];

      if (currCompileIndex != loc) {
        if (p) {
          Audio().Play(compile_item);
        }
        else {
          Audio().Play(compile_no_item);
        }

        currCompileIndex = loc;
      }

      if(p) {
        label = p->name;
      }

      size_t len = 1u + std::ceil(label.size() * text_progress);
      infoText.SetString("Running...\n" + label.substr(0, len));
    }

    return;
  }
  else {
    sf::Color color = progressBar.getColor();

    if (color.a > 0) {
      color.a -= 1;
      progressBarUVs.left -= 1; // another speedup at this step
      progressBar.setColor(color);
    }
  }

  if (questionInterface) {
    if (!textbox.IsOpen()) return;

    if (hasLeftInput()) {
      questionInterface->SelectYes();
      return;
    }

    if (hasRightInput()) {
      questionInterface->SelectNo();
      return;
    }

    if (Input().Has(InputEvents::pressed_confirm)) {
      if (!textbox.IsEndOfBlock()) {
        textbox.CompleteCurrentBlock();
        return;
      }

      questionInterface->ConfirmSelection();
      return;
    }
  }

  if (itemListSelected) {
    if (handleArrowKeys(elapsed)) {
      return;
    }

    //else arrow keys are not held this state
    selectInputCooldown = 0;
    extendedHold = false;

    if (!Input().Has(InputEvents::pressed_confirm)) return;
    
    if (handleSelectItemFromList()) return;

    auto onYes = [this]() {
      startCompile();
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
      pieces.push_back(insertingPiece);
      insertingPiece = nullptr;
      return;
    }

    if (Input().Has(InputEvents::pressed_confirm)) {
      if (insertPiece(insertingPiece, cursorLocation)) {
        insertingPiece = nullptr;
      }
      else {
        Audio().Play(AudioType::CHIP_ERROR, AudioPriority::lowest);
      }
      return;
    }

    if (Input().Has(InputEvents::pressed_shoulder_left)) {
      insertingPiece->rotateLeft();
      return;
    }
    
    if (Input().Has(InputEvents::pressed_shoulder_right)) {
      insertingPiece->rotateRight();
      return;
    }
  }
  else if (grabbedPiece) {
    if (Input().Has(InputEvents::pressed_cancel)) {
      insertPiece(grabbedPiece, grabStartLocation);
      cursorLocation = grabStartLocation;
      grabbedPiece = nullptr;
      updateCursorHoverInfo();
      return;
    }

    if (Input().Has(InputEvents::pressed_confirm)) {
      if (insertPiece(grabbedPiece, cursorLocation)) {
        grabbedPiece = nullptr;
      }
      else {
        Audio().Play(AudioType::CHIP_ERROR, AudioPriority::lowest);
      }
      return;
    }

    if (Input().Has(InputEvents::pressed_shoulder_left)) {
      grabbedPiece->rotateLeft();
      return;
    }
  }

  if (handleArrowKeys(elapsed)) {
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
    count++;
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
  float yoffset = 0.;
  
  if (listStart+1u > MAX_ITEMS_ON_SCREEN) {
    yoffset = -blueButtonSprite.getLocalBounds().height * ((listStart+1u) - MAX_ITEMS_ON_SCREEN);
  }

  sf::Vector2f previewPos{};
  sf::Vector2f bottom;
  bottom.x = 140.f;

  for (size_t i = 0; i < pieces.size(); i++) {
    blueButtonSprite.setPosition(bottom.x * blueButtonSprite.getScale().x, (bottom.y + yoffset) * blueButtonSprite.getScale().y);
    surface.draw(blueButtonSprite);

    if (listStart == i) {
      sf::Vector2f dest = blueButtonSprite.getPosition();
      sf::Vector2f pos = itemArrowCursor.getPosition();
      pos.x = swoosh::ease::interpolate(0.5f, pos.x, dest.x);
      pos.y = swoosh::ease::interpolate(0.5f, pos.y, dest.y);
      itemArrowCursor.setPosition(pos);
      previewPos = { dest.x + (blueButtonSprite.getGlobalBounds().width+10.f), dest.y };
    }

    sf::Vector2f textOffset = { 4.f, 8.f };
    itemText.SetString(pieces[i]->name);
    itemText.SetColor(sf::Color(33, 115, 140));
    itemText.setPosition((bottom.x + textOffset.x) * itemText.getScale().x, ((bottom.y + yoffset) * itemText.getScale().y) + textOffset.y + 2.f);
    surface.draw(itemText);
    itemText.SetColor(sf::Color::White);
    itemText.setPosition((bottom.x + textOffset.x) * itemText.getScale().x, ((bottom.y + yoffset) * itemText.getScale().y) + textOffset.y);
    surface.draw(itemText);

    bottom.y += blueButtonSprite.getLocalBounds().height;
  }

  if (bottom.y == 0) {
    bottom.y = blueButtonSprite.getLocalBounds().height;
  }

  // draw run button
  greenButtonSprite.setPosition(bottom.x * greenButtonSprite.getScale().x, (bottom.y+yoffset)*greenButtonSprite.getScale().y);
  surface.draw(greenButtonSprite);

  if (itemListSelected) {
    surface.draw(itemArrowCursor);

    if (listStart == pieces.size()) {
      sf::Vector2f dest = greenButtonSprite.getPosition();
      sf::Vector2f pos = itemArrowCursor.getPosition();
      pos.x = swoosh::ease::interpolate(0.5f, pos.x, dest.x);
      pos.y = swoosh::ease::interpolate(0.5f, pos.y, dest.y);
      itemArrowCursor.setPosition(pos);
    }
    else {
      // check if this value was set
      if (previewPos.x > 0.) {
        drawPreview(surface, pieces[listStart], previewPos);
      }
    }
  }

  // draw info box
  surface.draw(infoBox);

  sf::Vector2f pos = infoText.getPosition();
  infoText.SetColor(sf::Color(33, 115, 140));
  infoText.setPosition(pos.x+2.f, pos.y+2.f);
  surface.draw(infoText);
  infoText.SetColor(sf::Color::White);
  infoText.setPosition(pos);
  surface.draw(infoText);

  // progress bar shows when compiling
  surface.draw(progressBar);
  
  if(!itemListSelected) {
    // claw/cursor always draws on top
    Piece* p = insertingPiece ? insertingPiece : grabbedPiece ? grabbedPiece : nullptr;
    if (p) {
      drawPiece(surface, p, cursor.getPosition());

      if (grabbedPiece) {
        surface.draw(claw);
      }
    }
    else {
      // display any hovering text
      sf::Vector2f pos = hoverText.getPosition();
      hoverText.SetColor(sf::Color::Black);
      hoverText.setPosition(pos.x + 2.f, pos.y + 2.f);
      surface.draw(hoverText);
      hoverText.SetColor(sf::Color::White);
      hoverText.setPosition(pos);
      surface.draw(hoverText);

      if (grid[cursorLocation]) {
        surface.draw(claw);
      }
      else {
        surface.draw(cursor);
      }
    }
  }

  // textbox is top over everything
  surface.draw(textbox);
}

void PlayerCustScene::onEnd()
{
}