#include <Swoosh/ActivityController.h>
#include "bnBattleScene.h"
#include "bnChipLibrary.h"
#include "bnGameOverScene.h"
#include "bnUndernetBackground.h"
#include "bnWeatherBackground.h"
#include "bnRobotBackground.h"
#include "bnMedicalBackground.h"
#include "bnACDCBackground.h"
#include "bnMiscBackground.h"
#include "bnJudgeTreeBackground.h"
#include "bnPlayerHealthUI.h"
#include "Android/bnTouchArea.h"

#include "Segues/WhiteWashFade.h"
#include "Segues/PixelateBlackWashFade.h"

// modals like chip cust and battle reward slide in 12px per frame for 10 frames. 60 frames = 1 sec
// modal slide moves 120px in 1/6th of a second
// Per 1 second that is 6*120px in 6*1/6 of a sec = 720px in 1 sec
#define MODAL_SLIDE_PX_PER_SEC 720.0f

// Combos are counted if more than one enemy is hit within x frames
// The game is clocked to display 60 frames per second
// If x = 20 frames, then we want a combo hit threshold of 20/60 = 0.3 seconds
#define COMBO_HIT_THRESHOLD_SECONDS 20.0f/60.0f

BattleScene::BattleScene(swoosh::ActivityController& controller, Player* player, Mob* mob, ChipFolder* folder) :
        swoosh::Activity(&controller),
        player(player),
        mob(mob),
        lastMobSize(mob->GetMobCount()),
        didDoubleDelete(false),
        didTripleDelete(false),
        isChangingForm(false),
        isAnimatingFormChange(false),
        comboDeleteCounter(0),
        pauseShader(*SHADERS.GetShader(ShaderType::BLACK_FADE)),
        whiteShader(*SHADERS.GetShader(ShaderType::WHITE_FADE)),
        yellowShader(*SHADERS.GetShader(ShaderType::YELLOW)),
        customBarShader(*SHADERS.GetShader(ShaderType::CUSTOM_BAR)),
        heatShader(*SHADERS.GetShader(ShaderType::SPOT_DISTORTION)),
        iceShader(*SHADERS.GetShader(ShaderType::SPOT_REFLECTION)),
        distortionMap(*TEXTURES.GetTexture(TextureType::HEAT_TEXTURE)),
        summons(player),
        chipListener(player),
        // cap of 8 chips, 8 chips drawn per turn
        chipCustGUI(folder->Clone(), 8, 8), 
        camera(*ENGINE.GetCamera()),
        chipUI(player),
        lastSelectedForm(-1),
        persistentFolder(folder) {

  if (mob->GetMobCount() == 0) {
    Logger::Log(std::string("Warning: Mob was empty when battle started. Mob Type: ") + typeid(mob).name());
  }

  /*
  Set Scene*/
  field = mob->GetField();

  player->ChangeState<PlayerIdleState>();
  field->AddEntity(*player, 2, 2);

  // Chip UI for player
  chipListener.Subscribe(chipUI);
  summons.Subscribe(chipUI); // Let the scene's chip listener know about summon chips

  /*
  Background for scene*/
  background = mob->GetBackground();

  if (!background) {
    int randBG = rand() % 9;

    if (randBG == 0) {
      background = new LanBackground();
    }
    else if (randBG == 1) {
      background = new GraveyardBackground();
    }
    else if (randBG == 2) {
      background = new VirusBackground();
    }
    else if (randBG == 3) {
      background = new WeatherBackground();
    }
    else if (randBG == 4) {
      background = new RobotBackground();
    }
    else if (randBG == 5) {
      background = new MedicalBackground();
    }
    else if (randBG == 6) {
      background = new ACDCBackground();
    }
    else if (randBG == 7) {
      background = new MiscBackground();
    }
    else if (randBG == 8) {
      background = new JudgeTreeBackground();
    }
  }

  components = mob->GetComponents();

  PlayerHealthUI* healthUI = new PlayerHealthUI(player);
  chipCustGUI.AddNode(healthUI);
  components.push_back((UIComponent*)healthUI);

  for (auto c : components) {
    c->Inject(*this);
  }

  ProcessNewestComponents();

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

  camera = Camera(ENGINE.GetView());
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

  // Load forms
  chipCustGUI.SetPlayerFormOptions(player->GetForms());

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

  showSummonBackdrop = false;
  showSummonBackdropLength = 15.0/60.0; //15 frame fade
  showSummonBackdropTimer = 0; // No state is active
  showSummonText = false;
  summonTextLength = 1.25; // in seconds

  backdropOpacity = 0.25; // default is 25%

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

  heatShader.setUniform("texture", sf::Shader::CurrentTexture);
  heatShader.setUniform("distortionMapTexture", distortionMap);
  heatShader.setUniform("textureSizeIn", sf::Glsl::Vec2((float)textureSize.x, (float)textureSize.y));

  iceShader.setUniform("texture", sf::Shader::CurrentTexture);
  iceShader.setUniform("sceneTexture", sf::Shader::CurrentTexture);
  iceShader.setUniform("textureSizeIn", sf::Glsl::Vec2((float)textureSize.x, (float)textureSize.y));
  iceShader.setUniform("shine", 0.2f);

  shine = sf::Sprite(LOAD_TEXTURE(MOB_BOSS_SHINE));
  shine.setScale(2.f, 2.f);

  shineAnimation = Animation("resources/mobs/boss_shine.animation");
  shineAnimation.Load();

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
  // effectively returns all of them
  auto entities = field->FindEntities([](Entity* e) { return true; });

  for (auto e : entities) {
    if (e->components.size() > 0) {
      // update the ledger
      // Injects usually removes the owner so this step proceeds the lastComponentID update
      auto latestID = e->components[0]->GetID();

      if (e->lastComponentID < latestID) {
        //std::cout << "latestID: " << latestID << " lastComponentID: " << e->lastComponentID << "\n";

        for (auto c : e->components) {
          if (c->GetID() <= e->lastComponentID) break; // Older components are last in order, we're done

          // Otherwise inject into scene
          c->Inject(*this);
          // Logger::Log("component ID " + std::to_string(c->GetID()) + " was injected");
        }

        e->lastComponentID = latestID;
      }
    }
  }
}

const bool BattleScene::IsBattleActive()
{
  return !(isBattleRoundOver || (mob->GetRemainingMobCount() == 0) || isPaused || isInChipSelect || !mob->IsSpawningDone() || showSummonBackdrop || isPreBattle || isPostBattle || isChangingForm);
}

// TODO: this needs to be handled by some chip API itself and not hacked
void BattleScene::TEMPFilterAtkChips(Chip ** chips, int chipCount)
{
  // Only remove the ATK chips in the queue. Increase the previous chip damage by +10
  int newChipCount = chipCount;
  Chip* nonSupport = nullptr;

  // Create a temp chip list
  Chip** newChipList = new Chip*[chipCount];

  int j = 0;
  for (int i = 0; i < chipCount; ) {
    if (chips[i]->GetShortName() == "Atk+10") {
      if (nonSupport) {
        // Do not modify these chips
        auto supportChips = { "Barrier", "Invis", "Recov80" };
        if (std::find(supportChips.begin(), supportChips.end(), nonSupport->GetShortName()) ==  supportChips.end()) {
          nonSupport->damage += 10;
        }
      }

      i++;
      continue;
    }

    newChipList[j] = chips[i];
    nonSupport = chips[i];

    i++;
    j++;
  }

  newChipCount = j;

  // Set the new chips
  for (int i = 0; i < newChipCount; i++) {
    chips[i] = *(newChipList + i);
  }

  // Delete the temp list space
  // NOTE: We are _not_ deleting the pointers in them
  delete[] newChipList;

  this->chips = chips;
  this->chipCount = newChipCount;
}

void BattleScene::OnCounter(Character & victim, Character & aggressor)
{
  AUDIO.Play(AudioType::COUNTER, AudioPriority::HIGHEST);

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

  shineAnimation.Update((float)elapsed, shine);

  if(!isPaused) {
    this->summonTimer += elapsed;

    if (!isChangingForm) {
      if (showSummonBackdropTimer < showSummonBackdropLength && !summons.IsSummonActive() && showSummonBackdrop && prevSummonState) {
        showSummonBackdropTimer += elapsed;
        backdropOpacity = 0.25; // reset for summons

        //Logger::Log(std::string() + "showSummonBackdropTimer: " + std::to_string(showSummonBackdropTimer) + " showSummonBackdropLength: " + std::to_string(showSummonBackdropLength));
      }
      else if (showSummonBackdropTimer >= showSummonBackdropLength && !summons.IsSummonActive() && showSummonBackdrop && !showSummonText && prevSummonState) {
        if (!summons.IsSummonOver()) {
          showSummonText = true;
          //Logger::Log("showSummonText: " + (showSummonText ? std::string("true") : std::string("false")));
        }
      }
      else if (showSummonBackdropTimer > 0 && summons.IsSummonOver()) {
        showSummonBackdropTimer -= elapsed;
        showSummonText = prevSummonState = false;
        //Logger::Log(std::string() + "showSummonBackdropTimer: " + std::to_string(showSummonBackdropTimer) + " going to 0");

      }
      else if (showSummonBackdropTimer <= 0 && summons.IsSummonOver()) {
        showSummonBackdropTimer = 0;
        showSummonBackdrop = false;
      }
    }
    else if (isChangingForm && !isAnimatingFormChange) {
      showSummonBackdrop = true;
      player->Reveal(); // If flickering

      if (showSummonBackdropTimer < showSummonBackdropLength) {
        showSummonBackdropTimer += elapsed;
      } else if (showSummonBackdropTimer >= showSummonBackdropLength) {
        if (!isAnimatingFormChange) {
          isAnimatingFormChange = true;

          auto pos = player->getPosition();
          shine.setPosition(pos.x + 16.0f, pos.y - player->GetHitHeight()/4.0f);
          
          auto onTransform = [this]() {
            lastSelectedForm = chipCustGUI.GetSelectedFormIndex();
            player->ActivateFormAt(lastSelectedForm);
            AUDIO.Play(AudioType::SHINE);
          };

          auto onFinish = [this]() {
            isLeavingFormChange = true;
          };

          shineAnimation << "SHINE" << Animator::On(10, onTransform) << Animator::On(20, onFinish);
        }
      }
    }
    else if (isLeavingFormChange && showSummonBackdropTimer > 0.0f) {
      showSummonBackdropTimer -= elapsed*0.5f;

      if (showSummonBackdropTimer <= 0.0f) {
        isChangingForm = false; //done
        showSummonBackdrop = false;
        isLeavingFormChange = false;
        isAnimatingFormChange = false;

        // Show BattleStart
        isPreBattle = true;
        battleStartTimer.reset();
      }
    }
  }

  ProcessNewestComponents();

  // Update components
  for (auto c : components) {
    c->OnUpdate((float)elapsed);
  }

  chipUI.OnUpdate((float)elapsed);

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

  // Check if entire mob is deleted
  if (mob->IsCleared() && !isPlayerDeleted) {
    if (!isPostBattle && battleEndTimer.getElapsed().asSeconds() < postBattleLength) {
      // Show Enemy Deleted
      isPostBattle = true;
      battleEndTimer.reset();
      AUDIO.StopStream();
      AUDIO.Stream("resources/loops/enemy_deleted.ogg");
      player->ChangeState<PlayerIdleState>();
    }
    else if(!isBattleRoundOver && battleEndTimer.getElapsed().asSeconds() > postBattleLength) {
      isMobDeleted = true;
    }
  } else if (!isPlayerDeleted && mob->NextMobReady() && isSceneInFocus) {
    Mob::MobData* data = mob->GetNextMob();

    Agent* cast = dynamic_cast<Agent*>(data->mob);

    // Some entities have AI and need targets
    if (cast) {
      cast->SetTarget(player);
    }

    field->AddEntity(*data->mob, data->tileX, data->tileY);
    mobNames.push_back(data->mob->GetName());

    // Listen for counters
    this->Subscribe(*data->mob);
  }

  camera.Update((float)elapsed);

  background->Update((float)elapsed);

  // Do not update when: paused or in chip select, during a summon sequence, showing Battle Start sign
  if (!(isPaused || isInChipSelect || isChangingForm) && summons.IsSummonOver() && !isPreBattle) {


    // kill switch for testing:
    if (INPUT.Has(EventTypes::HELD_USE_CHIP) && INPUT.Has(EventTypes::HELD_SHOOT) && INPUT.Has(EventTypes::HELD_MOVE_LEFT)) {
      mob->KillSwitch();
    }

    field->Update((float)elapsed);
  } 

  int newMobSize = mob->GetRemainingMobCount();

  if (lastMobSize != newMobSize) {
    if (multiDeleteTimer.getElapsed() <= sf::seconds(COMBO_HIT_THRESHOLD_SECONDS) && !showSummonBackdrop && summons.IsSummonOver()) {
      comboDeleteCounter += lastMobSize - newMobSize;

      if (comboDeleteCounter == 2) {
        didDoubleDelete = true;
        comboInfo = doubleDelete;
        comboInfoTimer.reset();
      }
      else if (comboDeleteCounter > 2) {
        didTripleDelete = true;
        comboInfo = tripleDelete;
        comboInfoTimer.reset();
      }
    }
    else if(multiDeleteTimer.getElapsed() > sf::seconds(COMBO_HIT_THRESHOLD_SECONDS)){
      comboDeleteCounter = 0;
    }
  }

  if (lastMobSize != newMobSize) {
    // prepare for another enemy deletion
    multiDeleteTimer.reset();
  }

  lastMobSize = newMobSize;

  // TODO: we desperately need states
  // update the cust if not paused nor in chip select nor in mob intro nor battle results nor post battle
  if (!(isBattleRoundOver || (mob->GetRemainingMobCount() == 0) || isPaused || isInChipSelect || !mob->IsSpawningDone() || showSummonBackdrop || isPreBattle || isPostBattle || isChangingForm)) {
    if (battleTimer.isPaused()) {
      // start counting seconds again
      if (battleTimer.isPaused()) {
        battleTimer.start();
        comboDeleteCounter = 0; // reset the combo
        Logger::Log("comboDeleteCounter reset");
      }
    }

    if (newMobSize == 0) {
      if (!battleTimer.isPaused()) {
        battleTimer.pause();
        multiDeleteTimer.start();
        AUDIO.StopStream();
      }
    }

    customProgress += elapsed;

    // NOTE: this may be a redundant flag now that nodes and components can be updated by injection
    field->SetBattleActive(true);
  }
  else {
    battleTimer.pause();
    field->SetBattleActive(false);
  }

  chipCustGUI.Update((float)elapsed);

  // other player controls
  if (INPUT.Has(EventTypes::PRESSED_USE_CHIP) && !isInChipSelect && !isBattleRoundOver && summons.IsSummonOver() && !isPreBattle && !isPostBattle) {
    // TODO: move this to player controller state where these types of invasive checks are performed for us
    if (player && player->GetTile() && player->GetFirstComponent<AnimationComponent>()->GetAnimationString() == "PLAYER_IDLE") {
      chipUI.UseNextChip();
    }
  }
}

void BattleScene::onDraw(sf::RenderTexture& surface) {
  ENGINE.SetRenderSurface(surface);

  ENGINE.Clear();

  if (isChangingForm) {
    auto delta = 1.0 - (showSummonBackdropTimer / showSummonBackdropLength);

    background->setColor(sf::Color(int(255.f * delta), int(255.f * delta), int(255.f * delta), 255));
  }

  ENGINE.Draw(background);

  auto ui = std::vector<UIComponent*>();

  // First tile pass: draw the tiles
  Battle::Tile* tile = nullptr;

  auto allTiles = field->FindTiles([](Battle::Tile* tile) { return true; });
  auto tilesIter = allTiles.begin();

  while (tilesIter != allTiles.end()) {
    tile = (*tilesIter);

    // Skip edge tiles - they cannot be seen by players
    if (tile->IsEdgeTile()) {
      tilesIter++;
      continue;
    }

    tile->move(ENGINE.GetViewOffset());

    if (summons.IsSummonActive() || showSummonBackdrop || isChangingForm) {
      SpriteSceneNode* coloredTile = new SpriteSceneNode(*(sf::Sprite*)tile);
      coloredTile->SetShader(&pauseShader);
      pauseShader.setUniform("opacity", (float)backdropOpacity*float(std::max(0.0, (showSummonBackdropTimer / showSummonBackdropLength))));
      ENGINE.Draw(coloredTile);
      delete coloredTile;
    }
    else {
      if (tile->IsHighlighted()) {
        SpriteSceneNode* coloredTile = new SpriteSceneNode(*(sf::Sprite*)tile);
        coloredTile->SetShader(&yellowShader);
        ENGINE.Draw(coloredTile);
        delete coloredTile;
      }
      else {
        ENGINE.Draw(tile);
      }
    }

    tile->move(-ENGINE.GetViewOffset());
    tilesIter++;
  }

  // Second tile pass: draw the entities and shaders per row and per layer
  tile = nullptr;
  tilesIter = allTiles.begin();

  std::vector<Entity*> entitiesOnRow;
  int lastRow = 0;

  while (tilesIter != allTiles.end()) {
    if (lastRow != (*tilesIter)->GetY()) {
      lastRow = (*tilesIter)->GetY();

      // Ensure all entities are sorted by layer
      std::sort(entitiesOnRow.begin(), entitiesOnRow.end(), [](Entity* a, Entity*b) -> bool { return a->GetLayer() > b->GetLayer(); });

      // draw this row
      for (auto entity : entitiesOnRow) {
        entity->move(ENGINE.GetViewOffset());

        ENGINE.Draw(entity);

        entity->move(-ENGINE.GetViewOffset());
      }

      // prepare for bext row
      entitiesOnRow.clear();
    }

      tile = (*tilesIter);
      static float totalTime = 0;
      totalTime += (float)elapsed;

      //heatShader.setUniform("time", totalTime*0.02f);
      //heatShader.setUniform("distortionFactor", 0.01f);
      //heatShader.setUniform("riseFactor", 0.1f);

      //heatShader.setUniform("w", tile->GetWidth() - 8.f);
      //heatShader.setUniform("h", tile->GetHeight()*1.5f);

      //iceShader.setUniform("w", tile->GetWidth() - 8.f);
      //iceShader.setUniform("h", tile->GetHeight()*0.8f);

      Entity* entity = nullptr;

      auto allEntities = tile->FindEntities([](Entity* e) { return true; });
      auto entitiesIter = allEntities.begin();

      while (entitiesIter != allEntities.end()) {
          entity = (*entitiesIter);
        if (!entity->IsDeleted()) {
          auto uic = entity->GetComponentsDerivedFrom<UIComponent>();

          //Logger::Log("uic size is: " + std::to_string(uic.size()));

          if (!uic.empty()) {
            ui.insert(ui.begin(), uic.begin(), uic.end());
          }

          entitiesOnRow.push_back(*entitiesIter);

        }

          entitiesIter++;
      }

      /*if (tile->GetState() == TileState::LAVA) {
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
      }*/
        tilesIter++;
  }

  // Last row needs to be drawn now that the loop is over
  // Ensure all entities are sorted by layer
  std::sort(entitiesOnRow.begin(), entitiesOnRow.end(), [](Entity* a, Entity*b) -> bool { return a->GetLayer() > b->GetLayer(); });

  // draw this row
  for (auto entity : entitiesOnRow) {
    entity->move(ENGINE.GetViewOffset());

    ENGINE.Draw(entity);

    entity->move(-ENGINE.GetViewOffset());
  }

  // prepare for bext row
  entitiesOnRow.clear();

  // Draw scene nodes
  for (auto node : scenenodes) {
    surface.draw(*node);
  }

  // Draw ui
  //std::cout << "ui size: " << ui.size() << std::endl;

  for (auto node : ui) {
    surface.draw(*node);
  }

  if (isAnimatingFormChange) {
    surface.draw(shine);
  }

  // cust dissapears when not in battle
  if (!(isInChipSelect || isPostBattle || mob->IsCleared()))
    ENGINE.Draw(&customBarSprite);

  if (isPaused) {
    // apply shader on draw calls below
    ENGINE.SetShader(&pauseShader);
    pauseShader.setUniform("opacity", 0.25f);
  }

  // TODO: hack to swap out
  /*static auto lastShader = player->GetShader();

  if (!isInChipSelect && showSummonBackdropTimer > showSummonBackdropLength/25.0f) {
    player->SetShader(SHADERS.GetShader(ShaderType::WHITE));
  }
  else {
    player->SetShader(lastShader);
  }*/

  if (!summons.IsSummonActive() && showSummonText) {
    sf::Text summonsLabel = sf::Text(summons.GetSummonLabel(), *mobFont);

    double summonSecs = summonTimer - showSummonBackdropLength;
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
      summons.OnEnter();
      showSummonText = false;
    }
  }

  float nextLabelHeight = 0;
  if (!mob->IsCleared() && !mob->IsSpawningDone() || isInChipSelect) {
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


  if (!isPlayerDeleted && !showSummonBackdrop && !summons.IsSummonActive()) {
    ENGINE.Draw(chipUI);
  }

  if (isPreBattle && !isChangingForm) {
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

  // compare the summon state after we used a chip...
  if (!showSummonText) {
    if (summons.IsSummonOver() && prevSummonState == true) {
      // We are leaving the summons state this frame
      summons.OnLeave();
      prevSummonState = false;
    }
    else   // When these conditions are met, the chip name has shown and we're ready to follow through with the summon
    if (summons.IsSummonActive() && showSummonBackdrop) {
      summons.Update(elapsed);
    }

    // Track if a summon chip was used on this frame
    if (!prevSummonState) {
      prevSummonState = summons.HasMoreInQueue();

      if (prevSummonState) {
        summonTimer = 0;
        showSummonBackdrop = true;
        showSummonBackdropTimer = 0;
        std::cout << "prevSummonState flagged" << std::endl;
      }
    }
  }


  // Draw cust GUI on top of scene. No shaders affecting.
  ENGINE.Draw(chipCustGUI);

  // Scene keyboard controls
  // TODO: really belongs in Update() but also handles a lot of conditional draws
  //       refactoring battle scene into battle states should reduce this complexity
  if (INPUT.Has(EventTypes::PRESSED_PAUSE) && !isInChipSelect && !isChangingForm && !isBattleRoundOver && !isPreBattle && !isPostBattle) {
    isPaused = !isPaused;

    if (!isPaused) {
      ENGINE.RevokeShader();
    }
    else {
      AUDIO.Play(AudioType::PAUSE);
    }
  }
  else if ((!isMobFinished && mob->IsSpawningDone()) ||
           (
                   INPUT.Has(EventTypes::PRESSED_CUST_MENU) && customProgress >= customDuration && !isInChipSelect && !isPaused &&
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
  else if (isInChipSelect && chipCustGUI.IsInView() && chipCustGUI.CanInteract()) {
#ifndef __ANDROID__
    if (chipCustGUI.IsChipDescriptionTextBoxOpen()) {
      if (!INPUT.Has(EventTypes::HELD_QUICK_OPT)) {
        chipCustGUI.CloseChipDescription() ? AUDIO.Play(AudioType::CHIP_DESC_CLOSE, AudioPriority::LOWEST) : 1;
      }
      else if (INPUT.Has(EventTypes::PRESSED_CONFIRM) ){

        chipCustGUI.ChipDescriptionConfirmQuestion()? AUDIO.Play(AudioType::CHIP_CHOOSE) : 1;
        chipCustGUI.ContinueChipDescription();
      }

      if (INPUT.Has(EventTypes::HELD_CONFIRM)) {
        chipCustGUI.FastForwardChipDescription(3.0);
      }
      else {
        chipCustGUI.FastForwardChipDescription(1.0);
      }

      if (INPUT.Has(EventTypes::PRESSED_UI_LEFT)) {
        chipCustGUI.ChipDescriptionYes() ? AUDIO.Play(AudioType::CHIP_SELECT) : 1;;
      }
      else if (INPUT.Has(EventTypes::PRESSED_UI_RIGHT)) {
        chipCustGUI.ChipDescriptionNo() ? AUDIO.Play(AudioType::CHIP_SELECT) : 1;;
      }
    }
    else {
      if (INPUT.Has(EventTypes::PRESSED_UI_LEFT)) {
        chipSelectInputCooldown -= elapsed;

        if (chipSelectInputCooldown <= 0) {
          chipCustGUI.CursorLeft() ? AUDIO.Play(AudioType::CHIP_SELECT) : 1;
          chipSelectInputCooldown = maxChipSelectInputCooldown;
        }
      }
      else if (INPUT.Has(EventTypes::PRESSED_UI_RIGHT)) {
        chipSelectInputCooldown -= elapsed;

        if (chipSelectInputCooldown <= 0) {
          chipCustGUI.CursorRight() ? AUDIO.Play(AudioType::CHIP_SELECT) : 1;
          chipSelectInputCooldown = maxChipSelectInputCooldown;
        }
      }
      else if (INPUT.Has(EventTypes::PRESSED_UI_UP)) {
        chipSelectInputCooldown -= elapsed;

        if (chipSelectInputCooldown <= 0) {
          chipCustGUI.CursorUp() ? AUDIO.Play(AudioType::CHIP_SELECT) : 1;
          chipSelectInputCooldown = maxChipSelectInputCooldown;
        }
      }
      else if (INPUT.Has(EventTypes::PRESSED_UI_DOWN)) {
        chipSelectInputCooldown -= elapsed;

        if (chipSelectInputCooldown <= 0) {
          chipCustGUI.CursorDown() ? AUDIO.Play(AudioType::CHIP_SELECT) : 1;
          chipSelectInputCooldown = maxChipSelectInputCooldown;
        }
      }
      else {
        chipSelectInputCooldown = 0;
      }

      if (INPUT.Has(EventTypes::PRESSED_CONFIRM)) {
        bool performed = chipCustGUI.CursorAction();

        if (chipCustGUI.AreChipsReady()) {
          AUDIO.Play(AudioType::CHIP_CONFIRM, AudioPriority::HIGH);
          customProgress = 0; // NOTE: Temporary Hack. We base the cust state around the custom Progress value.
          //camera.MoveCamera(sf::Vector2f(240.f, 160.f), sf::seconds(0.5f));
        }
        else if (performed) {
          if (!chipCustGUI.SelectedNewForm()) {
            AUDIO.Play(AudioType::CHIP_CHOOSE, AudioPriority::HIGHEST);
          }
        }
        else {
          AUDIO.Play(AudioType::CHIP_ERROR, AudioPriority::LOWEST);
        }
      }
      else if (INPUT.Has(EventTypes::PRESSED_CANCEL) || sf::Mouse::isButtonPressed(sf::Mouse::Button::Right)) {
        chipCustGUI.CursorCancel() ? AUDIO.Play(AudioType::CHIP_CANCEL, AudioPriority::HIGHEST) : 1;
      }
      else if (INPUT.Has(EventTypes::HELD_QUICK_OPT)) {
        chipCustGUI.OpenChipDescription() ? AUDIO.Play(AudioType::CHIP_DESC, AudioPriority::LOWEST) : 1;
      }
    }

#else
    static bool isHidden = false;

    if(INPUT.Has(EventTypes::RELEASED_LEFT)) {
      if(!isHidden) {
        chipCustGUI.Hide();
        isHidden = true;
      } else {
        chipCustGUI.Reveal();
        isHidden = false;
      }
    }

    if (chipCustGUI.AreChipsReady() && !isHidden) {
      AUDIO.Play(AudioType::CHIP_CONFIRM, AudioPriority::HIGH);
      customProgress = 0; // NOTE: Temporary Hack. We base the cust state around the custom Progress value.
    }
#endif
  }

  if (isInChipSelect && customProgress > 0.f) {
    if (!chipCustGUI.IsInView()) {
      chipCustGUI.Move(sf::Vector2f(MODAL_SLIDE_PX_PER_SEC * (float)elapsed, 0));
    }
  }
  else {
    if (!chipCustGUI.IsOutOfView()) {
      chipCustGUI.Move(sf::Vector2f(-MODAL_SLIDE_PX_PER_SEC * (float)elapsed, 0));
    }
    else if (isInChipSelect) { // we're leaving a state
      // Start Program Advance checks
      if (isPAComplete && hasPA == -1) {
        // Filter and apply support chips
        TEMPFilterAtkChips(chips, chipCount);

        // Return to game
        isInChipSelect = false;
        chipUI.LoadChips(chips, chipCount);
        ENGINE.RevokeShader();

        int selectedForm = chipCustGUI.GetSelectedFormIndex();

        if (selectedForm != lastSelectedForm) {
          isChangingForm = true;
          showSummonBackdropTimer = 0;
          backdropOpacity = 1.0f; // full black
        }
        else {
          // Show BattleStart
          isPreBattle = true;
          battleStartTimer.reset();
        }
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

  if (isBattleRoundOver && !isPostBattle && !isPlayerDeleted && player->GetHealth() > 0) {
    if (!battleResults) {
      sf::Time totalBattleTime = battleTimer.getElapsed();

      battleResults = new BattleResults(totalBattleTime, player->GetMoveCount(), player->GetHitCount(), GetCounterCount(), didDoubleDelete, didTripleDelete, mob);
    }
    else {
      battleResults->Draw();

      if (!battleResults->IsInView()) {
        float amount = MODAL_SLIDE_PX_PER_SEC * (float)elapsed;
        battleResults->Move(sf::Vector2f(amount, 0));
      }
      else {
        if (INPUT.Has(EventTypes::PRESSED_CONFIRM)) {
          // Have to hit twice
          if (battleResults->IsFinished()) {
            BattleItem* reward = battleResults->GetReward();

            if (reward != nullptr) {
              if (reward->IsChip()) {
                // TODO: send the battle item off to the player's
                // persistent session storage (aka a save file or cloud database)
                CHIPLIB.AddChip(reward->GetChip());
                delete reward;
              }
            }

            using segue = swoosh::intent::segue<PixelateBlackWashFade, swoosh::intent::milli<500>>;
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
      using segue = swoosh::intent::segue<WhiteWashFade, swoosh::intent::milli<500>>::to<GameOverScene>;
      getController().queueRewind<segue>();
    }
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
  isSceneInFocus = true;

  // Stream battle music
  if (mob->HasCustomMusicPath()) {
    AUDIO.Stream(mob->GetCustomMusicPath(), true);
  }
  else {
    if (!mob->IsBoss()) {
      sf::Music::TimeSpan span;
      span.offset = sf::microseconds(84);
      span.length = sf::seconds(120.0f * 1.20668f);

      AUDIO.Stream("resources/loops/loop_battle.ogg", true, span);
    }
    else {
      AUDIO.Stream("resources/loops/loop_boss_battle.ogg", true);
    }
  }

#ifdef __ANDROID__
  this->SetupTouchControls();
#endif
}

void BattleScene::onLeave() {
#ifdef __ANDROID__
  this->ShutdownTouchControls();
#endif
}

void BattleScene::onExit() {
 // ENGINE.RevokeShader(); // Legacy code or necessary?
}

void BattleScene::onEnter() {
}

void BattleScene::onResume() {
#ifdef __ANDROID__
  this->SetupTouchControls();
#endif
}

void BattleScene::onEnd() {
  
#ifdef __ANDROID__
  this->ShutdownTouchControls();
#endif
}

#ifdef __ANDROID__
void BattleScene::SetupTouchControls() {
  /* Android touch areas*/
  TouchArea& rightSide = TouchArea::create(sf::IntRect(240, 0, 240, 320));

  rightSide.enableExtendedRelease(true);
  this->releasedB = false;

  rightSide.onTouch([]() {
      INPUT.VirtualKeyEvent(InputEvent::RELEASED_A);
  });

  rightSide.onRelease([this](sf::Vector2i delta) {
      if(!this->releasedB) {
        INPUT.VirtualKeyEvent(InputEvent::PRESSED_A);
      }

      this->releasedB = false;

  });

  rightSide.onDrag([this](sf::Vector2i delta){
      if(delta.x < -25 && !this->releasedB) {
        INPUT.VirtualKeyEvent(InputEvent::PRESSED_B);
        INPUT.VirtualKeyEvent(InputEvent::RELEASED_B);
        this->releasedB = true;
      }
  });

  rightSide.onDefault([this]() {
      this->releasedB = false;
  });

  TouchArea& custSelectButton = TouchArea::create(sf::IntRect(100, 0, 380, 100));
  custSelectButton.onTouch([]() {
      INPUT.VirtualKeyEvent(InputEvent::PRESSED_START);
  });
  custSelectButton.onRelease([](sf::Vector2i delta) {
      INPUT.VirtualKeyEvent(InputEvent::RELEASED_START);
  });

  TouchArea& dpad = TouchArea::create(sf::IntRect(0, 0, 240, 320));
  dpad.enableExtendedRelease(true);
  dpad.onDrag([](sf::Vector2i delta) {
      Logger::Log("dpad delta: " + std::to_string(delta.x) + ", " + std::to_string(delta.y));

      if(delta.x > 30) {
        INPUT.VirtualKeyEvent(InputEvent::PRESSED_RIGHT);
      }

      if(delta.x < -30) {
        INPUT.VirtualKeyEvent(InputEvent::PRESSED_LEFT);
      }

      if(delta.y > 30) {
        INPUT.VirtualKeyEvent(InputEvent::PRESSED_DOWN);
      }

      if(delta.y < -30) {
        INPUT.VirtualKeyEvent(InputEvent::PRESSED_UP);
      }
  });

  dpad.onRelease([](sf::Vector2i delta) {
      if(delta.x < -30) {
        INPUT.VirtualKeyEvent(InputEvent::RELEASED_LEFT);
      }
  });
}

void BattleScene::ShutdownTouchControls() {
  TouchArea::free();
}

#endif
