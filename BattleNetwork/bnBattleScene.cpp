#include <Swoosh\ActivityController.h>
#include "bnBattleScene.h"
#include "bnGameOverScene.h"
#include "bnUndernetBackground.h"
#include "bnPlayerHealthUI.h"

#include "Segues\WhiteWashFade.h"
#include "Segues\PixelateBlackwashFade.h"

BattleScene::BattleScene(swoosh::ActivityController& controller, Player* player, Mob* mob, ChipFolder* folder) :
  swoosh::Activity(controller), 
  player(player),
  mob(mob),
  lastMobSize(mob->GetMobCount()),
  didDoubleDelete(false),
  didTripleDelete(false),
  pauseShader(*SHADERS.GetShader(ShaderType::BLACK_FADE)),
  whiteShader(*SHADERS.GetShader(ShaderType::WHITE_FADE)),
  yellowShader(*SHADERS.GetShader(ShaderType::YELLOW)),
  customBarShader(*SHADERS.GetShader(ShaderType::CUSTOM_BAR)),
  heatShader(*SHADERS.GetShader(ShaderType::SPOT_DISTORTION)),
  iceShader(*SHADERS.GetShader(ShaderType::SPOT_REFLECTION)),
  distortionMap(*TEXTURES.GetTexture(TextureType::HEAT_TEXTURE)),
  summons(player),
  chipListener(player),
  chipCustGUI(folder, 8),
  camera(*ENGINE.GetCamera()),
  chipUI(player) {
  if (mob->GetMobCount() == 0) {
    Logger::Log(std::string("Warning: Mob was empty when battle started. Mob Type: ") + typeid(mob).name());
  }

  components = mob->GetComponents();

  PlayerHealthUI* healthUI = new PlayerHealthUI(player);
  player->RegisterComponent(healthUI);
  components.push_back(healthUI);
  scenenodes.push_back(dynamic_cast<SceneNode*>(healthUI));

  for (auto c : components) {
    c->Inject(*this);
  }

  // SCORE
  totalCounterDeletions = totalCounterMoves = 0;

  /*
  Program Advance + labels
  */
  programAdvance.LoadPA();
  isPAComplete = false;
  hasPA = -1;
  paStepIndex = 0;

  listStepCooldown = 0.2f;
  listStepCounter = listStepCooldown;

  programAdvanceSprite = sf::Sprite(LOAD_TEXTURE(PROGRAM_ADVANCE));
  programAdvanceSprite.setScale(2.f, 2.f);
  programAdvanceSprite.setOrigin(0, programAdvanceSprite.getLocalBounds().height/2.0f);
  programAdvanceSprite.setPosition(40.0f, 58.f);

  camera = Camera(ENGINE.GetDefaultView());
  ENGINE.SetCamera(camera);

  /* 
  Other battle labels
  */

  battleStart = sf::Sprite(LOAD_TEXTURE(BATTLE_START));
  battleStart.setOrigin(battleStart.getLocalBounds().width / 2.0f, battleStart.getLocalBounds().height / 2.0f);
  battleStartPos = sf::Vector2f(240.f, 140.f);
  battleStart.setPosition(battleStartPos);
  battleStart.setScale(2.f, 2.f);

  battleEnd = battleStart;
  battleEnd.setTexture(LOAD_TEXTURE(ENEMY_DELETED));

  doubleDelete = sf::Sprite(LOAD_TEXTURE(DOUBLE_DELETE));
  doubleDelete.setOrigin(doubleDelete.getLocalBounds().width / 2.0f, doubleDelete.getLocalBounds().height / 2.0f);
  comboInfoPos = sf::Vector2f(240.0f, 50.f);
  doubleDelete.setPosition(comboInfoPos);
  doubleDelete.setScale(2.f, 2.f);

  tripleDelete = doubleDelete;
  tripleDelete.setTexture(LOAD_TEXTURE(TRIPLE_DELETE));

  counterHit = doubleDelete;
  counterHit.setTexture(LOAD_TEXTURE(COUNTER_HIT));

  /*
  Chips + Chip select setup*/
  chips = nullptr;
  chipCount = 0;

  /*
  Battle results pointer */
  battleResults = nullptr;

  /*
  Set Scene*/
  field = mob->GetField();

  player->ChangeState<PlayerIdleState>();
  field->AddEntity(player, 2, 2);

  // Chip UI for player
  chipListener.Subscribe(chipUI);
  summons.Subscribe(chipUI); // Let the scene's chip listener know about summon chips

  /*
  Background for scene*/
  background = new UndernetBackground();;

  /*int randBG = rand() % 3;

  if (randBG == 0) {
    background = new LanBackground();
  }
  else if (randBG == 1) {
    background = new GraveyardBackground();
  }
  else if (randBG == 2) {
    background = new VirusBackground();
  }*/

  // PAUSE
  font = TEXTURES.LoadFontFromFile("resources/fonts/dr_cain_terminal.ttf");
  pauseLabel = new sf::Text("paused", *font);
  pauseLabel->setOrigin(pauseLabel->getLocalBounds().width / 2, pauseLabel->getLocalBounds().height * 2);
  pauseLabel->setPosition(sf::Vector2f(240.f, 160.f));

  // CHIP CUST GRAPHICS
  customBarTexture = TEXTURES.LoadTextureFromFile("resources/ui/custom.png");
  customBarSprite.setTexture(*customBarTexture);
  customBarSprite.setOrigin(customBarSprite.getLocalBounds().width / 2, 0);
  customBarPos = sf::Vector2f(240.f, 0.f);
  customBarSprite.setPosition(customBarPos);
  customBarSprite.setScale(2.f, 2.f);

  // Selection input delays
  maxChipSelectInputCooldown = 1 / 10.f; // tenth a second
  chipSelectInputCooldown = maxChipSelectInputCooldown;

  // MOB UI
  mobFont = TEXTURES.LoadFontFromFile("resources/fonts/mmbnthick_regular.ttf");

  // Stream battle music 
  if (!mob->IsBoss()) {
    AUDIO.Stream("resources/loops/loop_battle.ogg", true);
  }
  else {
    AUDIO.Stream("resources/loops/loop_boss_battle2.ogg", true);
  }
 
  // STATE FLAGS AND TIMERS
  isPaused = false;
  isInChipSelect = false;
  isChipSelectReady = false;
  isPlayerDeleted = false;
  isMobDeleted = false;
  isBattleRoundOver = false;
  isMobFinished = false;
  prevSummonState = false;
  customProgress = 0; // in seconds 
  customDuration = 10; // 10 seconds
  initFadeOut = false;

  isPreBattle = false;
  isPostBattle = false;
  preBattleLength = 1; // in seconds
  postBattleLength = 1.4; // in seconds
  PAStartLength = 0.15; // in seconds

  showSummonText = false;
  summonTextLength = 1; // in seconds

  // SHADERS
  // TODO: Load shaders if supported 
  shaderCooldown = 0;

  pauseShader.setUniform("texture", sf::Shader::CurrentTexture);
  pauseShader.setUniform("opacity", 0.25f);

  whiteShader.setUniform("texture", sf::Shader::CurrentTexture);
  whiteShader.setUniform("opacity", 0.5f);

  whiteShader.setUniform("texture", sf::Shader::CurrentTexture);

  customBarShader.setUniform("texture", sf::Shader::CurrentTexture);
  customBarShader.setUniform("factor", 0);
  customBarSprite.SetShader(&customBarShader);

  // Heat distortion effect
  distortionMap.setRepeated(true);
  distortionMap.setSmooth(true);

  textureSize = getController().getInitialWindowSize();

  heatShader.setUniform("currentTexture", sf::Shader::CurrentTexture);
  heatShader.setUniform("distortionMapTexture", distortionMap);
  heatShader.setUniform("textureSizeIn", sf::Glsl::Vec2((float)textureSize.x, (float)textureSize.y));

  iceShader.setUniform("currentTexture", sf::Shader::CurrentTexture);
  iceShader.setUniform("sceneTexture", sf::Shader::CurrentTexture);
  iceShader.setUniform("textureSizeIn", sf::Glsl::Vec2((float)textureSize.x, (float)textureSize.y));
  iceShader.setUniform("shine", 0.2f);
}

BattleScene::~BattleScene()
{
  components.clear();
  scenenodes.clear();
}

// What to do if we inject a chip publisher, subscribe it to the main listener
void BattleScene::Inject(ChipUsePublisher& pub)
{
  this->enemyChipListener.Subscribe(pub);
  SceneNode* node = dynamic_cast<SceneNode*>(&pub);
  this->scenenodes.push_back(node);
}

void BattleScene::OnCounter(Character & victim, Character & aggressor)
{
  AUDIO.Play(AudioType::COUNTER, AudioPriority::HIGH);

  // TODO: this is ready to be used but spells don't know who cast them, add that
  //if (&aggressor == this->player) {
    std::cout << "player countered" << std::endl;
    totalCounterMoves++;

    if (victim.IsDeleted()) {
      totalCounterDeletions++;
    }

    comboInfo = counterHit;
    comboInfoTimer.reset();
 // }
}

void BattleScene::onUpdate(double elapsed) {
  this->elapsed = elapsed;

  // Update components
  for (auto c : components) {
    c->Update((float)elapsed);
  }

  if (battleResults) {
    battleResults->Update(elapsed);
  }

  // check every frame 
  if (!isPlayerDeleted) {
    isPlayerDeleted = player->IsDeleted();

    if (isPlayerDeleted) {
      AUDIO.Play(AudioType::DELETED);
    }
  }

  isBattleRoundOver = (isPlayerDeleted || isMobDeleted);

  float FPS = 0.f;

  if ((float)elapsed > 0.f) {
    FPS = 1.0f / (float)elapsed;
    std::string fpsStr = std::to_string(FPS);
    fpsStr.resize(4);
    ENGINE.GetWindow()->setTitle(sf::String(std::string("FPS: ") + fpsStr));
  }

  if (mob->NextMobReady()) {
    Mob::MobData* data = mob->GetNextMob();

    Agent* cast = dynamic_cast<Agent*>(data->mob);

    if (cast) {
      cast->SetTarget(player);
    }

    field->AddEntity(data->mob, data->tileX, data->tileY);
    mobNames.push_back(data->mob->GetName());

    // Listen for counters
    this->Subscribe(*data->mob);
  }

  // Check if entire mob is deleted
  if (mob->IsCleared()) {
    if (!isPostBattle && battleEndTimer.getElapsed().asSeconds() < postBattleLength) {
      // Show Enemy Deleted 
      isPostBattle = true;
      battleEndTimer.reset();
      AUDIO.StopStream();
      AUDIO.Stream("resources/loops/enemy_deleted.ogg");
    }
    else if(!isBattleRoundOver && battleEndTimer.getElapsed().asSeconds() > postBattleLength) {
      isMobDeleted = true;
    }
  }

  camera.Update((float)elapsed);

  background->Update((float)elapsed);

  if (!showSummonText) {
    summons.Update(elapsed);
  }

  // TODO: Refactor. 
  // add player health component as a scene node child of cust gui
  // e.g.
  /*
    // Somewhere in the scene's INIT routine...
    Component* component = player->GetComponent<PlayerHealthUI>();
    SceneNode* healthUI = dynamic_cast<SceneNode*>(component); // is of both types
    if(healthUI) {
      chipCustGUI.AddSceneNode(healthUI); // healthUI will now offset in chipcustGUI local space
    }
  */
  player->GetComponent<PlayerHealthUI>()->OffsetPosition(chipCustGUI.GetOffset());

  // compare the summon state after we used a chip...
  if (summons.IsSummonsActive() && prevSummonState == false) {
    // We are switching over to a new state this frame
    summonTimer.reset();
    showSummonText = true;
  }
  else if (summons.IsSummonOver() && prevSummonState == true) {
    // We are leaving the summons state this frame
    summons.OnLeave();

  }

  // Do not update when: paused or in chip select, during a summon sequence, showing Battle Start sign
  if (!(isPaused || isInChipSelect) && summons.IsSummonOver() && !isPreBattle) {
    field->Update((float)elapsed);
  }

  // update the cust if not paused nor in chip select nor in mob intro nor battle results nor post battle
  if (!(isBattleRoundOver || (mob->GetRemainingMobCount() == 0) || isPaused || isInChipSelect || !mob->IsSpawningDone() || summons.IsSummonsActive() || isPreBattle || isPostBattle)) {  
    int newMobSize = mob->GetRemainingMobCount();

    if (lastMobSize != newMobSize) {
      if (lastMobSize - newMobSize == 2) {
        didDoubleDelete = true;
        comboInfo = doubleDelete;
        comboInfoTimer.reset();
      }
      else if (lastMobSize - newMobSize > 2) {
        didTripleDelete = true;
        comboInfo = tripleDelete;
        comboInfoTimer.reset();
      }

      lastMobSize = newMobSize;
    }

    if (newMobSize == 0) {
      if (!battleTimer.isPaused()) {
        battleTimer.pause();
        AUDIO.StopStream();
      }
    }
    else {
      if (battleTimer.isPaused()) {
        // start counting seconds again 
        battleTimer.start();
      }

      customProgress += elapsed;

      field->SetBattleActive(true);
    }
  }
  else {
    battleTimer.pause();
    field->SetBattleActive(false);
  }

  chipCustGUI.Update((float)elapsed);
}

void BattleScene::onDraw(sf::RenderTexture& surface) {
  ENGINE.SetRenderSurface(surface);

  ENGINE.Clear();

  ENGINE.Draw(background);

  sf::Vector2f cameraAntiOffset = -ENGINE.GetViewOffset();

  // First tile pass: draw the tiles
  Battle::Tile* tile = nullptr;
  while (field->GetNextTile(tile)) {
    tile->move(ENGINE.GetViewOffset());

    if (summons.IsSummonsActive()) {
      LayeredDrawable* coloredTile = new LayeredDrawable(*(sf::Sprite*)tile);
      coloredTile->SetShader(&pauseShader);
      ENGINE.Draw(coloredTile);
      delete coloredTile;
    }
    else {
      if (tile->IsHighlighted()) {
        LayeredDrawable* coloredTile = new LayeredDrawable(*(sf::Sprite*)tile);
        coloredTile->SetShader(&yellowShader);
        ENGINE.Draw(coloredTile);
        delete coloredTile;
      }
      else {
        ENGINE.Draw(tile);
      }
    }
  }

  miscComponents.clear();

  // Second tile pass: draw the entities and shaders per row
  tile = nullptr;
  while (field->GetNextTile(tile)) {

    static float totalTime = 0;
    totalTime += (float)elapsed;

    heatShader.setUniform("time", totalTime*0.02f);
    heatShader.setUniform("distortionFactor", 0.01f);
    heatShader.setUniform("riseFactor", 0.1f);

    heatShader.setUniform("w", tile->GetWidth() - 8.f);
    heatShader.setUniform("h", tile->GetHeight()*1.5f);

    iceShader.setUniform("w", tile->GetWidth() - 8.f);
    iceShader.setUniform("h", tile->GetHeight()*0.8f);

    Entity* entity = nullptr;

    while (tile->GetNextEntity(entity)) {
      if (!entity->IsDeleted()) {
        ENGINE.Draw(entity);
        miscComponents.push_back(entity->GetMiscComponents());
      }
    }

    if (tile->GetState() == TileState::LAVA) {
      heatShader.setUniform("x", tile->getPosition().x - tile->getTexture()->getSize().x + 3.0f);

      float repos = (float)(tile->getPosition().y - (tile->getTexture()->getSize().y*2.5f));
      heatShader.setUniform("y", repos);

      surface.display();
      sf::Texture postprocessing = surface.getTexture(); // Make a copy

      sf::Sprite distortionPost;
      distortionPost.setTexture(postprocessing);

      surface.clear();

      LayeredDrawable* bake = new LayeredDrawable(distortionPost);
      bake->SetShader(&heatShader);

      ENGINE.Draw(bake);
      delete bake;
    }
    else if (tile->GetState() == TileState::ICE) {
      iceShader.setUniform("x", tile->getPosition().x - tile->getTexture()->getSize().x);

      float repos = (float)(tile->getPosition().y - tile->getTexture()->getSize().y);
      iceShader.setUniform("y", repos);

      surface.display();
      sf::Texture postprocessing = surface.getTexture(); // Make a copy

      sf::Sprite reflectionPost;
      reflectionPost.setTexture(postprocessing);

      surface.clear();

      LayeredDrawable* bake = new LayeredDrawable(reflectionPost);
      bake->SetShader(&iceShader);

      ENGINE.Draw(bake);
      delete bake;
    }
  }

  /*Draw misc sprites*/
  for (auto list : miscComponents) {
    ENGINE.Draw(list);
  }

  // Draw scene nodes
  for (auto node : scenenodes) {
    node->OnDraw(surface);
  }

  /*for (int d = 1; d <= field->GetHeight(); d++) {
    Entity* entity = nullptr;
    while (field->GetNextEntity(entity, d)) {
      if (!entity->IsDeleted()) {
        ENGINE.Push(entity);
        ENGINE.Lay(entity->GetMiscComponents());
      }
    }
  }*/

  // NOTE: Although HUD, it fades dark when on chip cust screen and paused.
  if (!(isInChipSelect || isPostBattle || mob->IsCleared()))
    ENGINE.Draw(&customBarSprite);

  if (isPaused) {
    // apply shader on draw calls below
    ENGINE.SetShader(&pauseShader);
  }

  ENGINE.DrawOverlay();

  if (summons.IsSummonsActive() && showSummonText) {
    sf::Text summonsLabel = sf::Text(summons.GetSummonLabel(), *mobFont);

    double summonSecs = summonTimer.getElapsed().asSeconds();
    double scale = swoosh::ease::wideParabola(summonSecs, summonTextLength, 3.0);

    summonsLabel.setPosition(40.0f, 80.f);
    summonsLabel.setScale(1.0f, (float)scale);
    summonsLabel.setOutlineColor(sf::Color::Black);
    summonsLabel.setFillColor(sf::Color::White);
    summonsLabel.setOutlineThickness(2.f);
    summonsLabel.setOrigin(0, (summonsLabel.getLocalBounds().height * 2.0f) / 2.0f);
    ENGINE.Draw(summonsLabel, false);

    if (summonSecs >= summonTextLength) {
      summons.OnEnter();
      showSummonText = false;
    }
  }

  float nextLabelHeight = 0;
  if (!mob->IsSpawningDone() || isInChipSelect) {
    for (int i = 0; i < mob->GetMobCount(); i++) {
      if (mob->GetMobAt(i).IsDeleted())
        continue;

      sf::Text mobLabel = sf::Text(mob->GetMobAt(i).GetName(), *mobFont);

      mobLabel.setOrigin(mobLabel.getLocalBounds().width, 0);
      mobLabel.setPosition(470.0f, -1.f + nextLabelHeight);
      mobLabel.setScale(0.8f, 0.8f);
      mobLabel.setOutlineColor(sf::Color(48, 56, 80));
      mobLabel.setOutlineThickness(2.f);
      ENGINE.Draw(mobLabel, false);

      // make the next label relative to this one
      nextLabelHeight += mobLabel.getLocalBounds().height;
    }
  }


  if (!isPlayerDeleted && !summons.IsSummonsActive()) {
    chipUI.Update((float)elapsed); // DRAW 

    Drawable* component;
    while (chipUI.GetNextComponent(component)) {
      ENGINE.Draw(component);
    }
  }

  if (isPreBattle) {
    if (preBattleLength <= 0) {
      isPreBattle = false;
    }
    else {
      double battleStartSecs = battleStartTimer.getElapsed().asSeconds();
      double scale = swoosh::ease::wideParabola(battleStartSecs, preBattleLength, 2.0);
      battleStart.setScale(2.f, (float)scale*2.f);

      if (battleStartSecs >= preBattleLength)
        isPreBattle = false;

      ENGINE.Draw(battleStart);
    }
  }

  if (isPostBattle && !isBattleRoundOver) {
    if (postBattleLength <= 0) {
      isPostBattle = false;
    }
    else {
      double battleEndSecs = battleEndTimer.getElapsed().asSeconds();
      double scale = swoosh::ease::wideParabola(battleEndSecs, postBattleLength, 2.0);
      battleEnd.setScale(2.f, (float)scale*2.f);

      if (battleEndSecs >= postBattleLength) {
        isPostBattle = false;
      }

      ENGINE.Draw(battleEnd);
    }
  }

  if (comboInfoTimer.getElapsed().asSeconds() <= 1.0f) {
    ENGINE.Draw(comboInfo);
  }

  if (isPaused) {
    // render on top 
    ENGINE.Draw(pauseLabel, false);
  }

  // Track is a summon chip was used before key events below
  prevSummonState = summons.IsSummonsActive();

  // Draw cust GUI on top of scene. No shaders affecting.
  chipCustGUI.Draw();

  // Scene keyboard controls
  if (INPUT.has(PRESSED_PAUSE) && !isInChipSelect && !isBattleRoundOver && !isPreBattle && !isPostBattle) {
    isPaused = !isPaused;

    if (!isPaused) {
      ENGINE.RevokeShader();
    }
    else {
      AUDIO.Play(AudioType::PAUSE);
    }
  }
  else if (INPUT.has(RELEASED_B) && !isInChipSelect && !isBattleRoundOver && summons.IsSummonOver() && !isPreBattle && !isPostBattle) {
     chipUI.UseNextChip();
  }
  else if ((!isMobFinished && mob->IsSpawningDone()) || 
    (
      INPUT.has(PRESSED_START) && customProgress >= customDuration && !isInChipSelect && !isPaused && 
      !isBattleRoundOver && summons.IsSummonOver() && !isPreBattle && !isPostBattle
    )) {
    // enemy intro finished
    if (!isMobFinished) {
      // toggle the flag
      isMobFinished = true;
      // allow the player to be controlled by keys
      player->ChangeState<PlayerControlledState>();
      // Move mob out of the PixelInState 
      mob->DefaultState();
      // show the chip select screen
      customProgress = customDuration;
    }

    if (isInChipSelect == false && !isBattleRoundOver) {
      player->SetCharging(false);

      AUDIO.Play(AudioType::CUSTOM_SCREEN_OPEN);
      // slide up the screen a hair
      //camera.MoveCamera(sf::Vector2f(240.f, 140.f), sf::seconds(0.5f));
      isInChipSelect = true;

      // Clear any chip UI queues. they will contain null data. 
      chipUI.LoadChips(0, 0);

      // Reset PA system
      isPAComplete = false;
      hasPA = -1;
      paStepIndex = 0;
      listStepCounter = listStepCooldown;

      // Load the next chips
      chipCustGUI.ResetState();
      chipCustGUI.GetNextChips();
    }

    // NOTE: Need a battle scene state manager to handle going to and from one controll scheme to another. 
    // Plus would make more sense to revoke shaders once complete transition 

  }
  else if (isInChipSelect && chipCustGUI.IsInView()) {
    static bool A_HELD = false;

    if (chipCustGUI.IsChipDescriptionTextBoxOpen()) {
      if (!INPUT.has(HELD_PAUSE)) {
        chipCustGUI.CloseChipDescription() ? AUDIO.Play(AudioType::CHIP_DESC_CLOSE, AudioPriority::LOWEST) : 1;
      }
      else if (INPUT.has(PRESSED_A) ){

        chipCustGUI.ChipDescriptionConfirmQuestion()? AUDIO.Play(AudioType::CHIP_CHOOSE) : 1;
        chipCustGUI.ContinueChipDescription();
      }
      

      if (INPUT.has(HELD_A)) {
        chipCustGUI.FastForwardChipDescription(3.0);
      }
      else {
        chipCustGUI.FastForwardChipDescription(1.0);
      }

      if (INPUT.has(PRESSED_LEFT)) {
        chipCustGUI.ChipDescriptionYes() ? AUDIO.Play(AudioType::CHIP_SELECT) : 1;;
      }
      else if (INPUT.has(PRESSED_RIGHT)) {
        chipCustGUI.ChipDescriptionNo() ? AUDIO.Play(AudioType::CHIP_SELECT) : 1;;
      }
    }
    else {
      if (INPUT.has(PRESSED_LEFT)) {
        chipSelectInputCooldown -= elapsed;

        if (chipSelectInputCooldown <= 0) {
          chipCustGUI.CursorLeft() ? AUDIO.Play(AudioType::CHIP_SELECT) : 1;
          chipSelectInputCooldown = maxChipSelectInputCooldown;
        }
      }
      else if (INPUT.has(PRESSED_RIGHT)) {
        chipSelectInputCooldown -= elapsed;

        if (chipSelectInputCooldown <= 0) {
          chipCustGUI.CursorRight() ? AUDIO.Play(AudioType::CHIP_SELECT) : 1;
          chipSelectInputCooldown = maxChipSelectInputCooldown;
        }
      }
      else if (INPUT.has(PRESSED_UP)) {
        chipSelectInputCooldown -= elapsed;

        if (chipSelectInputCooldown <= 0) {
          chipCustGUI.CursorUp() ? AUDIO.Play(AudioType::CHIP_SELECT) : 1;
          chipSelectInputCooldown = maxChipSelectInputCooldown;
        }
      }
      else if (INPUT.has(PRESSED_DOWN)) {
        chipSelectInputCooldown -= elapsed;

        if (chipSelectInputCooldown <= 0) {
          chipCustGUI.CursorDown() ? AUDIO.Play(AudioType::CHIP_SELECT) : 1;
          chipSelectInputCooldown = maxChipSelectInputCooldown;
        }
      }
      else {
        chipSelectInputCooldown = 0;
      }

      if (INPUT.has(PRESSED_A)) {
        bool performed = chipCustGUI.CursorAction();

        if (chipCustGUI.AreChipsReady()) {
          AUDIO.Play(AudioType::CHIP_CONFIRM, AudioPriority::HIGH);
          customProgress = 0; // NOTE: Hack. Need one more state boolean
          //camera.MoveCamera(sf::Vector2f(240.f, 160.f), sf::seconds(0.5f)); 
        }
        else if (performed) {
          AUDIO.Play(AudioType::CHIP_CHOOSE, AudioPriority::HIGH);
        }
        else {
          AUDIO.Play(AudioType::CHIP_ERROR, AudioPriority::LOWEST);
        }
      }
      else if (INPUT.has(PRESSED_B)) {
        chipCustGUI.CursorCancel() ? AUDIO.Play(AudioType::CHIP_CANCEL, AudioPriority::HIGH) : 1;
      }
      else if (INPUT.has(PRESSED_PAUSE)) {
        chipCustGUI.OpenChipDescription() ? AUDIO.Play(AudioType::CHIP_DESC, AudioPriority::LOWEST) : 1;
      }
    }
  }

  if (isInChipSelect && customProgress > 0.f) {
    if (!chipCustGUI.IsInView()) {
      chipCustGUI.Move(sf::Vector2f(600.f * (float)elapsed, 0));
    }
  }
  else {
    if (!chipCustGUI.IsOutOfView()) {
      chipCustGUI.Move(sf::Vector2f(-600.f * (float)elapsed, 0));
    }
    else if (isInChipSelect) { // we're leaving a state
   // Start Program Advance checks
      if (isPAComplete && hasPA == -1) {
        // Return to game
        isInChipSelect = false;
        chipUI.LoadChips(chips, chipCount);
        ENGINE.RevokeShader();

        // Show BattleStart 
        isPreBattle = true;
        battleStartTimer.reset();
      }
      else if (!isPAComplete) {
        chips = chipCustGUI.GetChips();
        chipCount = chipCustGUI.GetChipCount();

        hasPA = programAdvance.FindPA(chips, chipCount);

        if (hasPA > -1) {
          paSteps = programAdvance.GetMatchingSteps();
          PAStartTimer.reset();
        }

        isPAComplete = true;
      }
      else if (hasPA > -1) {
        static bool advanceSoundPlay = false;
        static float increment = 0;
        
        float nextLabelHeight = 0;

        double PAStartSecs = PAStartTimer.getElapsed().asSeconds();
        double scale = swoosh::ease::linear(PAStartSecs, PAStartLength, 1.0);
        programAdvanceSprite.setScale(2.f, (float)scale*2.f);
        ENGINE.Draw(programAdvanceSprite, false);

        if (paStepIndex <= chipCount + 1) {
          for (int i = 0; i < paStepIndex && i < chipCount; i++) {
            std::string formatted = chips[i]->GetShortName();
            formatted.resize(9, ' ');
            formatted[8] = chips[i]->GetCode();

            sf::Text stepLabel = sf::Text(formatted, *mobFont);

            stepLabel.setOrigin(0, 0);
            stepLabel.setPosition(40.0f, 80.f + (nextLabelHeight*2.f));
            stepLabel.setScale(1.0f, 1.0f);

            if (i >= hasPA && i <= hasPA + paSteps.size() - 1) {
              if (i < paStepIndex - 1) {
                stepLabel.setOutlineColor(sf::Color(0, 0, 0));
                stepLabel.setFillColor(sf::Color(128, 248, 80));
              }
              else {
                stepLabel.setOutlineColor(sf::Color(0, 0, 0));
                stepLabel.setFillColor(sf::Color(247, 188, 27));
              }
            }
            else {
              stepLabel.setOutlineColor(sf::Color(48, 56, 80));
            }

            stepLabel.setOutlineThickness(2.f);
            ENGINE.Draw(stepLabel, false);

            // make the next label relative to this one
            nextLabelHeight += stepLabel.getLocalBounds().height;
          }
          increment = 0;
          nextLabelHeight = 0;
        }
        else {
          if (!advanceSoundPlay) {
            AUDIO.Play(AudioType::PA_ADVANCE);
            advanceSoundPlay = true;
          }

          for (int i = 0; i < chipCount; i++) {
            std::string formatted = chips[i]->GetShortName();
            formatted.resize(9, ' ');
            formatted[8] = chips[i]->GetCode();

            sf::Text stepLabel = sf::Text(formatted, *mobFont);

            stepLabel.setOrigin(0, 0);
            stepLabel.setPosition(40.0f, 80.f + (nextLabelHeight*2.f));
            stepLabel.setScale(1.0f, 1.0f);
            stepLabel.setOutlineColor(sf::Color(48, 56, 80));
            stepLabel.setOutlineThickness(2.f);

            if (i >= hasPA && i <= hasPA + paSteps.size() - 1) {
              if (i == hasPA) {
                Chip* paChip = programAdvance.GetAdvanceChip();

                sf::Text stepLabel = sf::Text(paChip->GetShortName(), *mobFont);
                stepLabel.setOrigin(0, 0);
                stepLabel.setPosition(40.0f, 80.f + (nextLabelHeight*2.f));
                stepLabel.setScale(1.0f, 1.0f);

                stepLabel.setOutlineColor(sf::Color((sf::Uint32)(sin(increment) * 255), (sf::Uint32)(cos(increment + 90 * (22.f / 7.f)) * 255), (sf::Uint32)(sin(increment + 180 * (22.f / 7.f)) * 255)));
                stepLabel.setOutlineThickness(2.f);
                ENGINE.Draw(stepLabel, false);
              }
              else {
                // make the next label relative to the hidden one and skip drawing
                nextLabelHeight += stepLabel.getLocalBounds().height;

                continue;
              }

            }
            else {
              ENGINE.Draw(stepLabel, false);
            }

            // make the next label relative to this one
            nextLabelHeight += stepLabel.getLocalBounds().height;
          }

          increment += (float)elapsed * 5.f;
        }

        if (listStepCounter > 0.f) {
          listStepCounter -= (float)elapsed;
        }
        else {
          // +2 = 1 step for showing PA label and 1 step for showing merged chip
          // That's the chips we want to show + 1 + 1 = chipCount + 2
          if (paStepIndex == chipCount + 2) {
            advanceSoundPlay = false;

            Chip* paChip = programAdvance.GetAdvanceChip();

            // Only remove the chips involved in the program advance. Replace them with the new PA chip.
            // PA chip is dealloc by the class that created it so it must be removed before the library tries to dealloc
            int newChipCount = chipCount - (int)paSteps.size() + 1; // Add the new one
            int newChipStart = hasPA;

            // Create a temp chip list
            Chip** newChipList = new Chip*[newChipCount];

            int j = 0;
            for (int i = 0; i < chipCount; ) {
              if (i == hasPA) {
                newChipList[j] = paChip;
                i += (int)paSteps.size();
                j++;
                continue;
              }

              newChipList[j] = chips[i];
              i++;
              j++;
            }

            // Set the new chips
            for (int i = 0; i < newChipCount; i++) {
              chips[i] = *(newChipList + i);
            }

            // Delete the temp list space 
            // NOTE: We are _not_ deleting the pointers in them
            delete[] newChipList;

            chipCount = newChipCount;

            hasPA = -1; // state over 
          }
          else {
            if (paStepIndex == chipCount + 1) {
              listStepCounter = listStepCooldown * 2.0f; // Linger on the screen when merged
            }
            else {
              listStepCounter = listStepCooldown * 0.7f; // Quicker about non-PA chips
            }

            if (paStepIndex >= hasPA && paStepIndex <= hasPA + paSteps.size() - 1) {
              listStepCounter = listStepCooldown; // Take our time with the PA chips 
              AUDIO.Play(AudioType::POINT);
            }

            paStepIndex++;
          }
        }
      }
    }
  }

  if (isBattleRoundOver && !isPostBattle && !isPlayerDeleted && player->GetHealth() > 0) {
    if (!battleResults) {
      sf::Time totalBattleTime = battleTimer.getElapsed();

      battleResults = new BattleResults(totalBattleTime, player->GetMoveCount(), player->GetHitCount(), GetCounterCount(), didDoubleDelete, didTripleDelete, mob);
    }
    else {
      battleResults->Draw();

      if (!battleResults->IsInView()) {
        float amount = 600.f * (float)elapsed;
        battleResults->Move(sf::Vector2f(amount, 0));
      }
      else {
        if (INPUT.has(PRESSED_A)) {
          // Have to hit twice
          if (battleResults->IsFinished()) {
            BattleItem* reward = battleResults->GetReward();
            
            if (reward != nullptr) {
              if (reward->IsChip()) {
                // TODO: sent the battle item off to the player's 
                // persistent session storage
                CHIPLIB.AddChip(reward->GetChip());
                delete reward;
              }
            }

            using segue = swoosh::intent::segue<PixelateBlackWashFade>;
            getController().queuePop<segue>();
          }
          else {
            battleResults->CursorAction();
          }
        }
      }
    }
  }
  else if (isBattleRoundOver && isPlayerDeleted) {
    if (!initFadeOut) {
      initFadeOut = true;
      using segue = swoosh::intent::segue<WhiteWashFade>::to<GameOverScene>;
      getController().queueRewind<segue>();
    }
  }

  // Write contents to screen (always last step)
  // ENGINE.Display();

  // TODO: make camera effects apply only to individual scenes that request them
  // This will avoid this hack here to move elements around on screen
  tile = nullptr;
  while (field->GetNextTile(tile)) {
    tile->move(cameraAntiOffset);
  }

  if (customProgress / customDuration >= 1.0) {
    if (isChipSelectReady == false) {
      AUDIO.Play(AudioType::CUSTOM_BAR_FULL);
      isChipSelectReady = true;
    }
  }
  else {
    isChipSelectReady = false;
  }

  customBarShader.setUniform("factor", (float)(customProgress / customDuration));
}

void BattleScene::onStart() {

}

void BattleScene::onLeave() {

}

void BattleScene::onExit() {
  ENGINE.RevokeShader();
}

void BattleScene::onEnter() {

}

void BattleScene::onResume() {

}

void BattleScene::onEnd() {

}