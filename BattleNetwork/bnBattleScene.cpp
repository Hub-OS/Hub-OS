#include <Swoosh/ActivityController.h>
#include "bnBattleScene.h"
#include "bnGameOverScene.h"
#include "bnUndernetBackground.h"
#include "bnPlayerHealthUI.h"

#include "Segues/WhiteWashFade.h"
#include "Segues/PixelateBlackWashFade.h"

BattleScene::BattleScene(swoosh::ActivityController& controller, Player* player, Mob* mob, ChipFolder* folder) :
  swoosh::Activity(&controller), 
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
  chipCustGUI(folder->Clone(), 8),
  camera(*ENGINE.GetCamera()),
  chipUI(player),
  persistentFolder(folder) {

  // Since mob sizes can be dynamic, it's currently possible to spawn no enemies
  // TODO: Is there a way to prevent this?
  if (mob->GetMobCount() == 0) {
    Logger::Log(std::string("Warning: Mob was empty when battle started. Mob Type: ") + typeid(mob).name());
  }

  /*
  Set Scene*/
  field = mob->GetField();

  // Have the player animate correctly, prevent keyboard input
  player->ChangeState<PlayerIdleState>();
  field->AddEntity(*player, 2, 2);

  // Chip UI for player
  chipListener.Subscribe(chipUI);
  summons.Subscribe(chipUI); // Let the scene's chip listener know about summon chips

  /*
  Background for scene*/
  background = mob->GetBackground();

  if (!background) {
    int randBG = rand() % 3;

    if (randBG == 0) {
      background = new LanBackground();
    }
    else if (randBG == 1) {
      background = new GraveyardBackground();
    }
    else if (randBG == 2) {
      background = new VirusBackground();
    }
  }

  components = mob->GetComponents();

  PlayerHealthUI* healthUI = new PlayerHealthUI(player);
  
  // Add the health UI to the chipcust GUI so when it slides out, so does the health
  chipCustGUI.AddNode(healthUI);
  
  // The component needs to be updated every loop
  components.push_back((UIComponent*)healthUI);

  // Handle special components
  for (auto c : components) {
    c->Inject(*this);
  }

  // Process components from entity creation
  ProcessNewestComponents();

  // Scoring defaults
  totalCounterDeletions = totalCounterMoves = 0;

  /*
  Program Advance + labels
  */
  programAdvance.LoadPA();
  isPAComplete = false;
  hasPA = -1;
  paStepIndex = 0;

  listStepCooldown = 0.2f; // in seconds
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

  textureSize = getController().getVirtualWindowSize();

  heatShader.setUniform("currentTexture", sf::Shader::CurrentTexture);
  heatShader.setUniform("distortionMapTexture", distortionMap);
  heatShader.setUniform("textureSizeIn", sf::Glsl::Vec2((float)textureSize.x, (float)textureSize.y));

  iceShader.setUniform("currentTexture", sf::Shader::CurrentTexture);
  iceShader.setUniform("sceneTexture", sf::Shader::CurrentTexture);
  iceShader.setUniform("textureSizeIn", sf::Glsl::Vec2((float)textureSize.x, (float)textureSize.y));
  iceShader.setUniform("shine", 0.2f);
  
  isSceneInFocus = false;
}

BattleScene::~BattleScene()
{
  components.clear();
  scenenodes.clear();
}

// What to do if we inject a chip publisher, subscribe it to the main listener
void BattleScene::Inject(ChipUsePublisher& pub)
{
  std::cout << "a chip use listener added" << std::endl;
  this->enemyChipListener.Subscribe(pub);
  this->summons.Subscribe(pub);

  SceneNode* node = dynamic_cast<SceneNode*>(&pub);
  this->scenenodes.push_back(node);
}

// what to do if we inject a UIComponent, add it to the update and topmost scenenode stack
void BattleScene::Inject(MobHealthUI& other)
{
  SceneNode* node = dynamic_cast<SceneNode*>(&other);
  this->scenenodes.push_back(node);
  components.push_back(&other);
}


// Default case: no special injection found for the type, just add it to our update loop
void BattleScene::Inject(Component * other)
{
  if (!other) return;

  components.push_back(other);
}

void BattleScene::Eject(Component * other)
{
  auto iter = std::find(components.begin(), components.end(), other);

  if (iter != components.end()) {
    components.erase(iter);
  }
}

void BattleScene::ProcessNewestComponents()
{
  // effectively returns all entities
  auto entities = field->FindEntities([](Entity* e) { return true; });

  for (auto e : entities) {
    if (e->shared.size() > 0) {
      // If the last component updated by scene is old
      if (e->lastComponentID < e->shared[0]->GetID()) {
        // Process the newest components
        for (auto c : e->shared) {
          // update the ledger 
          e->lastComponentID = e->shared[0]->GetID();

          // Inject usually removes the owner so this step proceeds the lastComponentID update
          c->Inject(*this);
        }
      }
    }
  }
}

// When an enemy gets countered by the player, play sound and show the text
void BattleScene::OnCounter(Character & victim, Character & aggressor)
{
  AUDIO.Play(AudioType::COUNTER, AudioPriority::HIGH);

  if (&aggressor == this->player) {
    totalCounterMoves++;

    if (victim.IsDeleted()) {
      totalCounterDeletions++;
    }

    comboInfo = counterHit;
    comboInfoTimer.reset();
  }
}

void BattleScene::onUpdate(double elapsed) {
  this->elapsed = elapsed;
  
  // TFC system has its own timer separate from main scene
  this->summonTimer += elapsed;

  // Process any newer components
  ProcessNewestComponents();

  // Update components first
  for (auto c : components) {
    c->Update((float)elapsed);
  }

  // If battleResults is created, update it
  if (battleResults) {
    battleResults->Update(elapsed);
  }

  // check every frame if player is deleted
  if (!isPlayerDeleted) {
    isPlayerDeleted = player->IsDeleted();

    if (isPlayerDeleted) {
      AUDIO.Play(AudioType::DELETED);
    }
  }

  // Battle is over if the player or the mob is deleted
  isBattleRoundOver = (isPlayerDeleted || isMobDeleted);

  /**
   * If the mob is spawning, NextMobReady() is true
   */
  if (mob->NextMobReady() && isSceneInFocus) {
    Mob::MobData* data = mob->GetNextMob();

    Agent* cast = dynamic_cast<Agent*>(data->mob);

    // Some entities have AI and need targets
    if (cast) {
      cast->SetTarget(player);
    }

    // Add to the field
    field->AddEntity(*data->mob, data->tileX, data->tileY);
    mobNames.push_back(data->mob->GetName());

    // Listen for counters
    this->Subscribe(*data->mob);
  }

  // Check if entire mob is deleted
  if (mob->IsCleared()) {
    // If the Enemy Deleted battle sign hasn't shown yet and the post battle timer isn't over
    if (!isPostBattle && battleEndTimer.getElapsed().asSeconds() < postBattleLength) {
      // Show Enemy Deleted sign
      isPostBattle = true;
      battleEndTimer.reset();
      AUDIO.StopStream();
      AUDIO.Stream("resources/loops/enemy_deleted.ogg");
      
      // Prevent keyboard input
      player->ChangeState<PlayerIdleState>();
    }
    else if(!isBattleRoundOver && battleEndTimer.getElapsed().asSeconds() > postBattleLength) {
      // When the Enemy Deleted Sign time has completed move onto the BattleRewards State
      isMobDeleted = true;
    }
  }

  camera.Update((float)elapsed);

  // Animate background
  background->Update((float)elapsed);

  // Do not update when: paused or in chip select, during a summon sequence, showing Battle Start sign
  // NOTE: IF THIS WAS A SEPERATE STATE THIS WOULD BE MINIMIZED
  if (!(isPaused || isInChipSelect) && summons.IsSummonOver() && !isPreBattle) {
    // kill switch for testing:
    if (INPUT.Has(InputEvent::PRESSED_A) && INPUT.Has(InputEvent::PRESSED_B) && INPUT.Has(InputEvent::PRESSED_LEFT) /*&& INPUT.Has(InputEvent::PRESSED_RIGHT)*/) {
        // Kills everything
        mob->KillSwitch();
    }

    // Update the field and all entities on it
    field->Update((float)elapsed);
  }

  // Check the mob size. Was it different last frame?
  int newMobSize = mob->GetRemainingMobCount();

  // Yes, enemies were deleted, double delete or triple delete?
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

  // todo: we need states
  // update the cust if not paused nor in chip select nor in mob intro nor battle results nor post battle
  if (!(isBattleRoundOver || (mob->GetRemainingMobCount() == 0) || isPaused || isInChipSelect || !mob->IsSpawningDone() || summons.IsSummonActive() || isPreBattle || isPostBattle)) {  
    int newMobSize = mob->GetRemainingMobCount();

    if (newMobSize == 0) {
      // If the battle active state is the current state and the mob size is 0
      // Pause the battle duration timer and stop the audio. 
      // We will be moving to a new state
      if (!battleTimer.isPaused()) {
        battleTimer.pause();
        AUDIO.StopStream();
      }
    }
    else {
      // If we have remaining enemies and the timer is paused (pauses during chip cust)
      if (battleTimer.isPaused()) {
        // start counting seconds again 
        battleTimer.start();
      }

      // Increase the cust gauge
      customProgress += elapsed;

      // Tell the field and all entities that the battle is active
      field->SetBattleActive(true);
    }
  }
  else {
    // Otherwise we might be in chip cust or PA
    // Pause the battle timer and tell the field that the battle is inactive
    battleTimer.pause();
    field->SetBattleActive(false);
  }

  chipCustGUI.Update((float)elapsed);

  // other player controls
  // Really hackey chip solution atm, if the player is IDLE
  // Then we can use the next chip
  // TODO: Use chip action to inform us when we need to allow chip use
  if (INPUT.Has(RELEASED_B) && !isInChipSelect && !isBattleRoundOver && summons.IsSummonOver() && !isPreBattle && !isPostBattle) {
    if (player && player->GetTile() && player->GetAnimationComponent().GetAnimationString() == "PLAYER_IDLE") {
      chipUI.UseNextChip();
    }
  }
}

void BattleScene::onDraw(sf::RenderTexture& surface) {
  // Use the external surface as our ENGINE's surface
  ENGINE.SetRenderSurface(surface);

  // Clear the surface
  ENGINE.Clear();

  // Draw the background first
  ENGINE.Draw(background);

  // The camera applies offset, we need to cancel that offset out
  // At the end of the frame
  sf::Vector2f cameraAntiOffset = -ENGINE.GetViewOffset();
  
  // Collect the UI components from the entities and draw them last
  auto ui = std::vector<UIComponent*>();

  // First tile pass: draw the tiles
  Battle::Tile* tile = nullptr;
  while (field->GetNextTile(tile)) {
    tile->move(ENGINE.GetViewOffset());

    // Dim the tiles only
    if (summons.IsSummonActive()) {
      SpriteSceneNode* coloredTile = new SpriteSceneNode(*(sf::Sprite*)tile);
      coloredTile->SetShader(&pauseShader);
      ENGINE.Draw(coloredTile);
      delete coloredTile;
    }
    else {
      // Turn tiles yellow
      if (tile->IsHighlighted()) {
        SpriteSceneNode* coloredTile = new SpriteSceneNode(*(sf::Sprite*)tile);
        coloredTile->SetShader(&yellowShader);
        ENGINE.Draw(coloredTile);
        delete coloredTile;
      }
      else {
        // Otherwise draw normally
        ENGINE.Draw(tile);
      }
    }
  }

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
        auto uic = entity->GetComponent<UIComponent>();
        if (uic) {
          // Grab UI components and add them to the last draw list
          ui.push_back(uic);
        }

        ENGINE.Draw(entity);
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

      SpriteSceneNode* bake = new SpriteSceneNode(distortionPost);
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

      SpriteSceneNode* bake = new SpriteSceneNode(reflectionPost);
      bake->SetShader(&iceShader);

      ENGINE.Draw(bake);
      delete bake;
    }
  }

  // Draw scene nodes
  for (auto node : scenenodes) {
    surface.draw(*node);
  }

  // Draw ui
  //std::cout << "ui size: " << ui.size() << std::endl;

  // Draw scene nodes
  for (auto node : ui) {
    surface.draw(*node);
  }


  // cust guage dissapears when not in battle
  if (!(isInChipSelect || isPostBattle || mob->IsCleared()))
    ENGINE.Draw(&customBarSprite);

  if (isPaused) {
    // apply shader on draw calls below
    ENGINE.SetShader(&pauseShader);
  }

  // State: TFC triggered by chip
  if (showSummonText) {
    sf::Text summonsLabel = sf::Text(summons.GetSummonLabel(), *mobFont);

    double summonSecs = summonTimer;
    double scale = swoosh::ease::wideParabola(summonSecs, summonTextLength, 3.0);

    if (summons.GetCallerTeam() == Team::RED) {
      summonsLabel.setPosition(40.0f, 80.f);
    }
    else {
      summonsLabel.setPosition(470.0f, 80.0f);
    }

    summonsLabel.setScale(1.0f, (float)scale);
    summonsLabel.setOutlineColor(sf::Color::Black);
    summonsLabel.setFillColor(sf::Color::White);
    summonsLabel.setOutlineThickness(2.f);

    if (summons.GetCallerTeam() == Team::RED) {
      summonsLabel.setOrigin(0, summonsLabel.getLocalBounds().height);
    }
    else {
      summonsLabel.setOrigin(summonsLabel.getLocalBounds().width, summonsLabel.getLocalBounds().height);
    }

    ENGINE.Draw(summonsLabel, false);

    if (summonSecs >= summonTextLength) {
      // Enter TFC
      summons.OnEnter();
      prevSummonState = true;
      showSummonText = false;
    }
  }

  float nextLabelHeight = 0;
  
  // When spawning show enemy names in a listed order
  // Also show names when in chip select GUI
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

  // Show player chip UI if player is still alive 
  // Do not show chips in TFC
  if (!isPlayerDeleted && !summons.IsSummonActive()) {
    chipUI.Update((float)elapsed); // DRAW 

    // TODO: we have a real component system now, refactor this
    Drawable* component;
    while (chipUI.GetNextComponent(component)) {
      ENGINE.Draw(component);
    }
  }

  // Pre/Post Battle "Battle Begin" and "Enemy Deleted" signs that appear for a few frames
  if (isPreBattle) {
    if (preBattleLength <= 0) {
      isPreBattle = false;
    }
    else {
      // Shrink the sign at end and beginning
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
      // Shrink the sign at end and beginning        
      double battleEndSecs = battleEndTimer.getElapsed().asSeconds();
      double scale = swoosh::ease::wideParabola(battleEndSecs, postBattleLength, 2.0);
      battleEnd.setScale(2.f, (float)scale*2.f);

      if (battleEndSecs >= postBattleLength) {
        isPostBattle = false;
      }

      ENGINE.Draw(battleEnd);
    }
  }

  // Show DoubleDelete or TripleDelete if activated
  if (comboInfoTimer.getElapsed().asSeconds() <= 1.0f) {
    ENGINE.Draw(comboInfo);
  }

  // State: show paused label if activated
  if (isPaused) {
    // render on top 
    ENGINE.Draw(pauseLabel, false);
  }

  // compare the summon state after we used a chip...
  if (!showSummonText) {
    if (summons.IsSummonOver() && prevSummonState == true) {
      // We are leaving the summons state this frame
      summons.OnLeave();
      prevSummonState = false;
    }
    else   // When these conditions are met, the chip name has shown and we're ready to follow through with the summon
      if (summons.IsSummonActive() && prevSummonState == true) {
        summons.Update(elapsed);
      }

    // Track if a summon chip was used on this frame
    if (!prevSummonState) {
      prevSummonState = summons.IsSummonActive() || summons.HasMoreInQueue();

      if (prevSummonState) {
        summonTimer = 0;
        showSummonText = true;
        std::cout << "prevSummonState flagged" << std::endl;
      }
    }
  }


  // Draw cust GUI on top of scene. No shaders affecting.
  ENGINE.Draw(chipCustGUI);

  // Scene keyboard controls
  /** @important TODO: really belongs in Update() but also handles a lot of conditional draws
    * refactoring battle scene into battle states should reduce this complexity
    **/
  if (INPUT.Has(PRESSED_PAUSE) && !isInChipSelect && !isBattleRoundOver && !isPreBattle && !isPostBattle) {
    isPaused = !isPaused;

    if (!isPaused) {
      ENGINE.RevokeShader();
    }
    else {
      AUDIO.Play(AudioType::PAUSE);
    }
  } else if ((!isMobFinished && mob->IsSpawningDone()) || 
    (
      INPUT.Has(PRESSED_START) && customProgress >= customDuration && !isInChipSelect && !isPaused && 
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
    // Plus would make more sense to revoke screen effects and labels once complete transition 

  }
  else if (isInChipSelect && chipCustGUI.IsInView()) 
      
    // Interact with the Chip Cust GUI's textbox
    // GUI provides API we must check that each action was a success
    
    if (chipCustGUI.IsChipDescriptionTextBoxOpen()) {
      if (!INPUT.Has(HELD_PAUSE)) {
        chipCustGUI.CloseChipDescription() ? AUDIO.Play(AudioType::CHIP_DESC_CLOSE, AudioPriority::LOWEST) : 1;
      }
      else if (INPUT.Has(PRESSED_A) ){

        chipCustGUI.ChipDescriptionConfirmQuestion()? AUDIO.Play(AudioType::CHIP_CHOOSE) : 1;
        chipCustGUI.ContinueChipDescription();
      }
      

      if (INPUT.Has(HELD_A)) {
        chipCustGUI.FastForwardChipDescription(3.0);
      }
      else {
        chipCustGUI.FastForwardChipDescription(1.0);
      }

      if (INPUT.Has(PRESSED_LEFT)) {
        chipCustGUI.ChipDescriptionYes() ? AUDIO.Play(AudioType::CHIP_SELECT) : 1;;
      }
      else if (INPUT.Has(PRESSED_RIGHT)) {
        chipCustGUI.ChipDescriptionNo() ? AUDIO.Play(AudioType::CHIP_SELECT) : 1;;
      }
    }
    else {
        
      // Interact with the Chipcust GUI 
      // Select left/right/up/down 
      // Select OK
      
      if (INPUT.Has(PRESSED_LEFT)) {
        chipSelectInputCooldown -= elapsed;

        if (chipSelectInputCooldown <= 0) {
          chipCustGUI.CursorLeft() ? AUDIO.Play(AudioType::CHIP_SELECT) : 1;
          chipSelectInputCooldown = maxChipSelectInputCooldown;
        }
      }
      else if (INPUT.Has(PRESSED_RIGHT)) {
        chipSelectInputCooldown -= elapsed;

        if (chipSelectInputCooldown <= 0) {
          chipCustGUI.CursorRight() ? AUDIO.Play(AudioType::CHIP_SELECT) : 1;
          chipSelectInputCooldown = maxChipSelectInputCooldown;
        }
      }
      else if (INPUT.Has(PRESSED_UP)) {
        chipSelectInputCooldown -= elapsed;

        if (chipSelectInputCooldown <= 0) {
          chipCustGUI.CursorUp() ? AUDIO.Play(AudioType::CHIP_SELECT) : 1;
          chipSelectInputCooldown = maxChipSelectInputCooldown;
        }
      }
      else if (INPUT.Has(PRESSED_DOWN)) {
        chipSelectInputCooldown -= elapsed;

        if (chipSelectInputCooldown <= 0) {
          chipCustGUI.CursorDown() ? AUDIO.Play(AudioType::CHIP_SELECT) : 1;
          chipSelectInputCooldown = maxChipSelectInputCooldown;
        }
      }
      else {
        chipSelectInputCooldown = 0;
      }

      if (INPUT.Has(PRESSED_A)) {
        
        // If chipcust GUI OK is selected, query that the chips are ready and then set the state over
        bool performed = chipCustGUI.CursorAction();

        if (chipCustGUI.AreChipsReady()) {
          AUDIO.Play(AudioType::CHIP_CONFIRM, AudioPriority::HIGH);
          customProgress = 0; // NOTE: Temporary Hack. We base the cust state around the custom Progress value.
          //camera.MoveCamera(sf::Vector2f(240.f, 160.f), sf::seconds(0.5f)); 
        }
        else if (performed) {
          AUDIO.Play(AudioType::CHIP_CHOOSE, AudioPriority::HIGH);
        }
        else {
          AUDIO.Play(AudioType::CHIP_ERROR, AudioPriority::LOWEST);
        }
      }
      else if (INPUT.Has(PRESSED_B) || sf::Mouse::isButtonPressed(sf::Mouse::Button::Right)) {
        chipCustGUI.CursorCancel() ? AUDIO.Play(AudioType::CHIP_CANCEL, AudioPriority::HIGH) : 1;
      }
      else if (INPUT.Has(PRESSED_PAUSE)) {
        chipCustGUI.OpenChipDescription() ? AUDIO.Play(AudioType::CHIP_DESC, AudioPriority::LOWEST) : 1;
      }
    }
  }

  // State: InChipSelect but the GUI is not in view, move it in view
  if (isInChipSelect && customProgress > 0.f) {
    if (!chipCustGUI.IsInView()) {
      chipCustGUI.Move(sf::Vector2f(1200.f * (float)elapsed, 0));
    }
  }
  else {
    // Otherwise move out of view
    if (!chipCustGUI.IsOutOfView()) {
      chipCustGUI.Move(sf::Vector2f(-1200.f * (float)elapsed, 0));
    }
    else if (isInChipSelect) { // we're leaving a state
   // Start Program Advance checks
      if (isPAComplete && hasPA == -1) {
        // State: PA Check end
        // Return to game
        isInChipSelect = false;
        chipUI.LoadChips(chips, chipCount);
        ENGINE.RevokeShader();

        // Show BattleStart 
        isPreBattle = true;
        battleStartTimer.reset();
      }
      else if (!isPAComplete) {
          
        // State: PA check begin
        chips = chipCustGUI.GetChips();
        chipCount = chipCustGUI.GetChipCount();

        // Check for PA
        hasPA = programAdvance.FindPA(chips, chipCount);

        // If hasPA > -1, we have a PA
        // Get steps and start the timer
        if (hasPA > -1) {
          paSteps = programAdvance.GetMatchingSteps();
          PAStartTimer.reset();
        }

        // Exit this state
        isPAComplete = true;
      }
      else if (hasPA > -1) {
        // State: Has PA
        
        static bool advanceSoundPlay = false;
        static float increment = 0;
        
        float nextLabelHeight = 0;

        double PAStartSecs = PAStartTimer.getElapsed().asSeconds();
        double scale = swoosh::ease::linear(PAStartSecs, PAStartLength, 1.0);
        programAdvanceSprite.setScale(2.f, (float)scale*2.f);
        ENGINE.Draw(programAdvanceSprite, false);

        // List, color, and format the chips selected from chip cust
        if (paStepIndex <= chipCount + 1) {
          for (int i = 0; i < paStepIndex && i < chipCount; i++) {
            std::string formatted = chips[i]->GetShortName();
            formatted.resize(9, ' ');
            formatted[8] = chips[i]->GetCode();

            sf::Text stepLabel = sf::Text(formatted, *mobFont);

            // Non PA chips are white
            stepLabel.setOrigin(0, 0);
            stepLabel.setPosition(40.0f, 80.f + (nextLabelHeight*2.f));
            stepLabel.setScale(1.0f, 1.0f);
 
            // If we're listing a chip in the PA
            if (i >= hasPA && i <= hasPA + paSteps.size() - 1) {
              // If we've already shown the PA step, make it green
              if (i < paStepIndex - 1) {
                stepLabel.setOutlineColor(sf::Color(0, 0, 0));
                stepLabel.setFillColor(sf::Color(128, 248, 80));
              }
              else {
                // If the current step counter is on a PA chip, make it orange
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
          // State: PA merged
          if (!advanceSoundPlay) {
            AUDIO.Play(AudioType::PA_ADVANCE);
            advanceSoundPlay = true;
          }

          // Create a blank space for the chips that were removed during the merge
          // Color all the other chips white
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
              // If the current list step is on the start of the PA
              // This chip is now the PA chip
              // Show the name and make it flash
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
              // Otherwise draw non PA chip white
              ENGINE.Draw(stepLabel, false);
            }

            // make the next label relative to this one
            nextLabelHeight += stepLabel.getLocalBounds().height;
          }

          increment += (float)elapsed * 5.f;
        }

        // Wait a few frames before listing the next chip
        if (listStepCounter > 0.f) {
          listStepCounter -= (float)elapsed;
        }
        else {
          // Why +2? B/c 1 step for showing PA label and 1 step for showing merged chip
          // That is equal to the # chips we want to show + 1 + 1 = chipCount + 2
          if (paStepIndex == chipCount + 2) {
            advanceSoundPlay = false;

            // State: Ending the list state
            // Get the PA chip from the progamAdvance module
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
            // listStepCounter <= 0.0f
            // State: listing state
            
            // If we're listing a non PA chip, move over them faster
            // PA chips linger on the screen for a bit
            
            if (paStepIndex == chipCount + 1) {
              listStepCounter = listStepCooldown * 2.0f; // Linger on the screen when showing the final PA
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

  // State: Player Win
  if (isBattleRoundOver && !isPostBattle && !isPlayerDeleted && player->GetHealth() > 0) {
    // Create the results object
    if (!battleResults) {
      sf::Time totalBattleTime = battleTimer.getElapsed();

      battleResults = new BattleResults(totalBattleTime, player->GetMoveCount(), player->GetHitCount(), GetCounterCount(), didDoubleDelete, didTripleDelete, mob);
    }
    else {
      battleResults->Draw();

      if (!battleResults->IsInView()) {
        float amount = 1200.f * (float)elapsed;
        battleResults->Move(sf::Vector2f(amount, 0));
      }
      else {
        if (INPUT.Has(PRESSED_A)) {
          // Have to hit twice, once to reveal item, second to end scene
          if (battleResults->IsFinished()) {
            BattleItem* reward = battleResults->GetReward();
            
            if (reward != nullptr) {
              if (reward->IsChip()) {
                // TODO: send the battle item off to the player's 
                // persistent session storage (aka a save file or cloud database)
                CHIPLIB.AddChip(reward->GetChip());
                Chip filtered = CHIPLIB.GetChipEntry(reward->GetChip().GetShortName(), reward->GetChip().GetCode());
                persistentFolder->AddChip(filtered);
                delete reward;
              }
            }

            // Tell the scene controller to pixelate out of this scene to the previous one
            using segue = swoosh::intent::segue<PixelateBlackWashFade>;
            getController().queuePop<segue>();
          }
          else {
            // Interact with the GUI api
            battleResults->CursorAction();
          }
        }
      }
    }
  }
  else if (isBattleRoundOver && isPlayerDeleted) {
    // State: Player Deleted
    if (!initFadeOut) {
      initFadeOut = true;
      
      // Unwind the scene stack to the game over scene (first scene on stack)
      using segue = swoosh::intent::segue<WhiteWashFade>::to<GameOverScene>;
      getController().queueRewind<segue>();
    }
  }

  tile = nullptr;
  // Move the tile back to the original state before cameraAntiOffset was applied
  while (field->GetNextTile(tile)) {
    tile->move(cameraAntiOffset);
  }

  // If cust is full, play sound, tell chip select GUI we can open it
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
  // Scene can start intro state
  isSceneInFocus = true;
	
  // Stream battle music 
  if (mob->HasCustomMusicPath()) {
    AUDIO.Stream(mob->GetCustomMusicPath(), true);
  }
  else {
    if (!mob->IsBoss()) {
      AUDIO.Stream("resources/loops/loop_battle.ogg", true);
    }
    else {
      AUDIO.Stream("resources/loops/loop_boss_battle.ogg", true);
    }
  }
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
