#include <SFML/Graphics.hpp>
using sf::RenderWindow;
using sf::VideoMode;
using sf::Clock;
using sf::Event;
using sf::Font;

#include <time.h>
#include <typeinfo>
#include "bnBattleScene.h"
#include "bnMob.h"
#include "bnField.h"
#include "bnPlayer.h"
#include "bnSelectedChipsUI.h"
#include "bnPlayerChipUseListener.h"
#include "bnChipSummonHandler.h"
#include "bnMemory.h"
#include "bnMettaur.h"
#include "bnProgsMan.h"
#include "bnLanBackground.h"
#include "bnGraveyardBackground.h"
#include "bnVirusBackground.h"
#include "bnPlayerHealthUI.h"
#include "bnCamera.h"
#include "bnInputManager.h"
#include "bnChipSelectionCust.h"
#include "bnChipFolder.h"
#include "bnTimer.h"
#include "bnShaderResourceManager.h"
#include "bnPA.h"
#include "bnEngine.h"
#include "bnBattleResults.h"

int BattleScene::Run(Player* player, Mob* mob) {
  if (mob->GetMobCount() == 0) {
    Logger::Log(std::string("Warning: Mob was empty when battle started. Mob Type: ") + typeid(mob).name());
    return 1; // signal win 
  }

  /*
  Program Advance + labels
  */
  PA programAdvance;
  PASteps paSteps;
  programAdvance.LoadPA();
  bool isPAComplete = false;
  int hasPA = -1;
  int paStepIndex = 0;

  float listStepCooldown = 0.5f;
  float listStepCounter = listStepCooldown;

  sf::Sprite programAdvanceSprite(LOAD_TEXTURE(PROGRAM_ADVANCE));
  programAdvanceSprite.setScale(2.f, 2.f);
  programAdvanceSprite.setOrigin(0, 0);
  programAdvanceSprite.setPosition(40.0f, 50.f);

  /*
  Mob labels*/
  std::vector<std::string> mobNames;

  Camera camera(ENGINE.GetDefaultView());
  ENGINE.SetCamera(camera);

  /* 
  Other battle labels
  */

  sf::Sprite battleStart(LOAD_TEXTURE(BATTLE_START));
  battleStart.setOrigin(battleStart.getLocalBounds().width / 2, battleStart.getLocalBounds().height / 2);
  sf::Vector2f battleStartPos = sf::Vector2f(240.f, 140.f);
  battleStart.setPosition(battleStartPos);
  battleStart.setScale(2.f, 2.f);
  Timer battleStartTimer;

  /*
  Chips + Chip select setup*/
  ChipFolder* folder = new ChipFolder(); // TODO: this would be read in from a file
  ChipSelectionCust chipCustGUI(folder, 8);
  Chip** chips = 0;
  int chipCount = 0;

  /*
  Chip Summons*/
  ChipSummonHandler summons(player);

  /*
  Battle results pointer */
  BattleResults* battleResults = nullptr;
  Timer battleTimer;

  /*
  Set Scene*/
  Field* field = mob->GetField();

  player->StateChange<PlayerIdleState>();
  field->AddEntity(player, 2, 2);

  PlayerHealthUI* playerHealthUI = player->GetHealthUI();

  // Chip UI for player
  SelectedChipsUI chipUI(player);
  PlayerChipUseListener chipListener(player);
  chipListener.Subscribe(chipUI);
  summons.Subscribe(chipUI); // Let the scene's chip listener know about summon chips

  /*
  Background for scene*/
  Background* background = nullptr;

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

  // PAUSE
  sf::Font* font = TEXTURES.LoadFontFromFile("resources/fonts/dr_cain_terminal.ttf");
  sf::Text* pauseLabel = new sf::Text("paused", *font);
  pauseLabel->setOrigin(pauseLabel->getLocalBounds().width / 2, pauseLabel->getLocalBounds().height * 2);
  pauseLabel->setPosition(sf::Vector2f(240.f, 160.f));

  // CHIP CUST GRAPHICS
  sf::Texture* customBarTexture = TEXTURES.LoadTextureFromFile("resources/ui/custom.png");
  LayeredDrawable customBarSprite;
  customBarSprite.setTexture(*customBarTexture);
  customBarSprite.setOrigin(customBarSprite.getLocalBounds().width / 2, 0);
  sf::Vector2f customBarPos = sf::Vector2f(240.f, 0.f);
  customBarSprite.setPosition(customBarPos);
  customBarSprite.setScale(2.f, 2.f);

  // Selection input delays
  double maxChipSelectInputCooldown = 1 / 10.f; // tenth a second
  double chipSelectInputCooldown = maxChipSelectInputCooldown;

  // MOB UI
  sf::Font *mobFont = TEXTURES.LoadFontFromFile("resources/fonts/mmbnthick_regular.ttf");

  // Stream battle music 
  if (!mob->IsBoss()) {
    AUDIO.Stream("resources/loops/loop_battle.ogg", true);
  }
  else {
    AUDIO.Stream("resources/loops/loop_boss_battle.ogg", true);
  }

  Clock clock;
  float elapsed = 0.0f;
  bool isPaused = false;
  bool isInChipSelect = false;
  bool isChipSelectReady = false;
  bool isPlayerDeleted = false;
  bool isMobDeleted = false;
  bool isBattleRoundOver = false;
  bool isMobFinished = false;
  bool prevSummonState = false;
  double customProgress = 0; // in seconds 
  double customDuration = 10; // 10 seconds
  bool initFadeOut = false;

  bool isPreBattle = false;
  double preBattleLength = 1; // in seconds

  Timer summonTimer;
  bool showSummonText = false;
  double summonTextLength = 1; // in seconds

  // Special: Load shaders if supported 
  double shaderCooldown = 0;

  sf::Shader& pauseShader = *SHADERS.GetShader(ShaderType::BLACK_FADE);
  pauseShader.setUniform("texture", sf::Shader::CurrentTexture);
  pauseShader.setUniform("opacity", 0.5f);

  sf::Shader& whiteShader = *SHADERS.GetShader(ShaderType::WHITE_FADE);
  whiteShader.setUniform("texture", sf::Shader::CurrentTexture);
  whiteShader.setUniform("opacity", 0.5f);

  sf::Shader& yellowShader = *SHADERS.GetShader(ShaderType::YELLOW);
  whiteShader.setUniform("texture", sf::Shader::CurrentTexture);

  sf::Shader& customBarShader = *SHADERS.GetShader(ShaderType::CUSTOM_BAR);
  customBarShader.setUniform("texture", sf::Shader::CurrentTexture);
  customBarShader.setUniform("factor", 0);
  customBarSprite.SetShader(&customBarShader);

  // Heat distortion effect
  sf::Texture& distortionMap = *TEXTURES.GetTexture(TextureType::HEAT_TEXTURE);
  distortionMap.setRepeated(true);
  distortionMap.setSmooth(true);

  sf::Vector2u textureSize = ENGINE.GetPostProcessingBuffer().getSize();

  sf::Shader& heatShader = *SHADERS.GetShader(ShaderType::SPOT_DISTORTION);
  heatShader.setUniform("currentTexture", sf::Shader::CurrentTexture);
  heatShader.setUniform("distortionMapTexture", distortionMap);
  heatShader.setUniform("textureSizeIn", sf::Glsl::Vec2(textureSize.x, textureSize.y));

  sf::Shader& iceShader = *SHADERS.GetShader(ShaderType::SPOT_REFLECTION);
  iceShader.setUniform("currentTexture", sf::Shader::CurrentTexture);
  iceShader.setUniform("sceneTexture", sf::Shader::CurrentTexture);
  iceShader.setUniform("textureSizeIn", sf::Glsl::Vec2(textureSize.x, textureSize.y));
  iceShader.setUniform("shine", 0.3f);

  std::vector<std::vector<Drawable*>> miscComponents;

  bool inBattleState = true;
  float totalTime = 0;
  while (ENGINE.Running() && inBattleState) {
    // check every frame 
    if (!isPlayerDeleted) {
      isPlayerDeleted = player->IsDeleted();

      if (isPlayerDeleted) {
        AUDIO.Play(AudioType::DELETED);
      }
    }

    isBattleRoundOver = (isPlayerDeleted || isMobDeleted);

    float elapsedSeconds = clock.restart().asSeconds();

    totalTime += elapsedSeconds;

    float FPS = 0.f;

    if (elapsedSeconds > 0.f) {
      FPS = 1.0f / elapsedSeconds;
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
    }

    // Check if entire mob is deleted
    if (mob->IsCleared()) {
      isMobDeleted = true; // Hack. Just to trigger fade out and spawn a new mob
    }

    INPUT.update();

    camera.Update(elapsed);

    background->Update(elapsed);

    if (!showSummonText) {
      summons.Update(elapsed);
    }

    // TODO: Refactor. Health UI should be a fetchable component (not a dedicated function)
    // Positions and offsets would make more sense if these components were also of type SceneNode
    // So they would always gaurantee to have translation functions
    // e.g.
    /*
      // Somewhere in the scene's INIT routine...
      Battle::Component* healthUI = player->GetComponent<PlayerHealthUI>();
      if(healthUI) {
        chipCustGUI.AddSceneNode(healthUI); // healthUI will now offset in chipcustGUI local space
      }
    */
    player->GetHealthUI()->OffsetPosition(chipCustGUI.GetOffset());

    // compare the summon state after we used a chip...
    if (summons.IsSummonsActive() && prevSummonState == false) {
      // We are switching over to a new state this frame
      summonTimer.Reset();
      showSummonText = true;
    }
    else if (summons.IsSummonOver() && prevSummonState == true) {
      // We are leaving the summons state this frame
      summons.OnLeave();

    }

    // Do not update when: paused or in chip select, during a summon sequence, showing Battle Start sign
    if (!(isPaused || isInChipSelect ) && summons.IsSummonOver() && !isPreBattle) {
      field->Update(elapsed);
    }

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

      heatShader.setUniform("time", totalTime);
      heatShader.setUniform("distortionFactor", 0.01f);
      heatShader.setUniform("riseFactor", 0.02f);

      heatShader.setUniform("w", tile->GetWidth() - 8.f);
      heatShader.setUniform("h", tile->GetHeight()*1.5f);

      iceShader.setUniform("w", tile->GetWidth() - 8.f);
      iceShader.setUniform("h", tile->GetHeight()*0.9f);

      Entity* entity = nullptr;

      while (tile->GetNextEntity(entity)) {
        if (!entity->IsDeleted()) {
          ENGINE.Draw(entity);
          miscComponents.push_back(entity->GetMiscComponents());
        }
      }

      if (tile->GetState() == TileState::LAVA) {
        heatShader.setUniform("x", tile->getPosition().x + 4.f);

        float repos = (float)(tile->getPosition().y - 4.f) - (tile->GetHeight() / 1.5f);
        heatShader.setUniform("y", repos);

        sf::Texture postprocessing = ENGINE.GetPostProcessingBuffer().getTexture(); // Make a copy

        sf::Sprite distortionPost;
        distortionPost.setTexture(postprocessing);

        LayeredDrawable* bake = new LayeredDrawable(distortionPost);
        bake->SetShader(&heatShader);

        ENGINE.Draw(bake);
        delete bake;
      }
      else if (tile->GetState() == TileState::ICE) {
        iceShader.setUniform("x", tile->getPosition().x + 4.f);

        float repos = (float)(tile->getPosition().y - 4.f);
        iceShader.setUniform("y", repos);

        sf::Texture postprocessing = ENGINE.GetPostProcessingBuffer().getTexture(); // Make a copy

        sf::Sprite reflectionPost;
        reflectionPost.setTexture(postprocessing);

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

    ENGINE.Draw(playerHealthUI, false);

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
    if (!isBattleRoundOver && !isInChipSelect)
      ENGINE.Draw(&customBarSprite);

    if (isPaused || isInChipSelect) {
      // apply shader on draw calls below
      ENGINE.SetShader(&pauseShader);
    } 

    ENGINE.DrawOverlay();

    if (summons.IsSummonsActive() && showSummonText) {
      sf::Text summonsLabel = sf::Text(summons.GetSummonLabel(), *mobFont);

      double summonSecs = summonTimer.GetElapsed() / 1000.0;
      double scale = Ease::WideParabola(summonSecs, summonTextLength);

      summonsLabel.setPosition(40.0f, 40.f);
      summonsLabel.setScale(1.0f, (float)scale);
      summonsLabel.setOutlineColor(sf::Color::Black);
      summonsLabel.setFillColor(sf::Color::White);
      summonsLabel.setOutlineThickness(2.f);
      summonsLabel.setOrigin(summonsLabel.getGlobalBounds().width / 2.0f, summonsLabel.getGlobalBounds().height / 2.0f);
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


    if (!isPlayerDeleted) {
      chipUI.Update(); // DRAW 

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
        double battleStartSecs = battleStartTimer.GetElapsed() / 1000.0;
        double scale = Ease::WideParabola(battleStartSecs, preBattleLength);
        battleStart.setScale(2.f, (float)scale*2.f);

        if (battleStartSecs >= preBattleLength)
          isPreBattle = false;

        ENGINE.Draw(battleStart);
     }
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
    if (INPUT.has(PRESSED_PAUSE) && !isInChipSelect && !isBattleRoundOver && !isPreBattle) {
      isPaused = !isPaused;

      if (!isPaused) {
        ENGINE.RevokeShader();
      }
      else {
        AUDIO.Play(AudioType::PAUSE);
      }
    } else if (INPUT.has(RELEASED_B) && !isInChipSelect && !isBattleRoundOver && summons.IsSummonOver() && !isPreBattle) {
      chipUI.UseNextChip();
    } else if ((!isMobFinished && mob->IsSpawningDone()) || (INPUT.has(PRESSED_START) && customProgress >= customDuration && !isInChipSelect && !isBattleRoundOver && summons.IsSummonOver() && !isPreBattle)) {
       // enemy intro finished
      if (!isMobFinished) { 
        // toggle the flag
        isMobFinished = true; 
        // allow the player to be controlled by keys
        player->StateChange<PlayerControlledState>(); 
        // Move mob out of the PixelInState 
        mob->DefaultState();
        // show the chip select screen
        customProgress = customDuration; 
      }
      
      if (isInChipSelect == false && !isBattleRoundOver) {
        player->SetCharging(false);

        AUDIO.Play(AudioType::CHIP_SELECT);
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

    } else if (isInChipSelect && chipCustGUI.IsInView()) {
      if (INPUT.has(PRESSED_LEFT)) {
        chipSelectInputCooldown -= elapsed;

        if (chipSelectInputCooldown <= 0) {
          chipCustGUI.CursorLeft() ? AUDIO.Play(AudioType::CHIP_SELECT) : 1;
          chipSelectInputCooldown = maxChipSelectInputCooldown;
        }
      } else if (INPUT.has(PRESSED_RIGHT)) {
        chipSelectInputCooldown -= elapsed;

        if (chipSelectInputCooldown <= 0) {
          chipCustGUI.CursorRight() ? AUDIO.Play(AudioType::CHIP_SELECT) : 1;
          chipSelectInputCooldown = maxChipSelectInputCooldown;
        }
      } else if (INPUT.has(PRESSED_UP)) {
        chipSelectInputCooldown -= elapsed;

        if (chipSelectInputCooldown <= 0) {
          chipCustGUI.CursorUp() ? AUDIO.Play(AudioType::CHIP_SELECT) : 1;
          chipSelectInputCooldown = maxChipSelectInputCooldown;
        }
      } else if (INPUT.has(PRESSED_DOWN)) {
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
          AUDIO.Play(AudioType::CHIP_CONFIRM, AudioPriority::LOWEST);
          customProgress = 0; // NOTE: Hack. Need one more state boolean
          //camera.MoveCamera(sf::Vector2f(240.f, 160.f), sf::seconds(0.5f)); 
        } else if(performed){
          AUDIO.Play(AudioType::CHIP_CHOOSE, AudioPriority::LOWEST);
        }
        else {
          AUDIO.Play(AudioType::CHIP_ERROR, AudioPriority::LOWEST);
        }
      } else if (INPUT.has(PRESSED_B)) {
        chipCustGUI.CursorCancel() ? AUDIO.Play(AudioType::CHIP_CANCEL, AudioPriority::LOWEST) : 1;
      }
    }

    if (isInChipSelect && customProgress > 0.f) {
      if (!chipCustGUI.IsInView()) {
        chipCustGUI.Move(sf::Vector2f(600.f * elapsed, 0));
      }
    } else {
      if (!chipCustGUI.IsOutOfView()) {
        chipCustGUI.Move(sf::Vector2f(-600.f * elapsed, 0));
      } else if (isInChipSelect) { // we're leaving a state
        // Start Program Advance checks
        if(isPAComplete && hasPA == -1) {
          // Return to game
          isInChipSelect = false;
          chipUI.LoadChips(chips, chipCount);
          ENGINE.RevokeShader();

          // Show BattleStart 
          isPreBattle = true;
          battleStartTimer.Reset();
        }
        else if (!isPAComplete) {
          chips = chipCustGUI.GetChips();
          chipCount = chipCustGUI.GetChipCount();

          hasPA = programAdvance.FindPA(chips, chipCount);

          if(hasPA > -1) {
            paSteps = programAdvance.GetMatchingSteps();
          }

          isPAComplete = true;
        }
        else if (hasPA > -1) {
          static bool advanceSoundPlay = false;
          static float increment = 0;

          float nextLabelHeight = 0;

          ENGINE.Draw(programAdvanceSprite, false);

          if (paStepIndex <= chipCount+1) {
            for (int i = 0; i < paStepIndex && i < chipCount; i++) {
              std::string formatted = chips[i]->GetShortName();
              formatted.resize(9, ' ');
              formatted[8] = chips[i]->GetCode();

              sf::Text stepLabel = sf::Text(formatted, *mobFont);

              stepLabel.setOrigin(0, 0);
              stepLabel.setPosition(40.0f, 80.f + (nextLabelHeight*2.f));
              stepLabel.setScale(1.0f, 1.0f);

              if (i >= hasPA && i <= hasPA + paSteps.size()-1) {
                if (i < paStepIndex-1) {
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
          }
          else {
            if (!advanceSoundPlay) {
              AUDIO.Play(AudioType::PA_ADVANCE);
              advanceSoundPlay = true;
            }

            increment += elapsed * 5.f;

            Chip* paChip = programAdvance.GetAdvanceChip();

            sf::Text stepLabel = sf::Text(paChip->GetShortName(), *mobFont);

            stepLabel.setOrigin(0, 0);
            stepLabel.setPosition(40.0f, 80.f);
            stepLabel.setScale(1.0f, 1.0f);
            stepLabel.setOutlineColor(sf::Color((sf::Uint32)(sin(increment) * 255), (sf::Uint32)(cos(increment+90*(22.f/7.f)) * 255), (sf::Uint32)(sin(increment+180*(22.f/7.f)) * 255)));
            stepLabel.setOutlineThickness(2.f);
            ENGINE.Draw(stepLabel, false);
          }

          if (listStepCounter > 0.f) {
            listStepCounter -= elapsed;
          }
          else {
            if (paStepIndex == chipCount+2) {
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
              listStepCounter = listStepCooldown * 0.7f; // Quicker about non-PA chips

              if (paStepIndex >= hasPA && paStepIndex <= hasPA + paSteps.size() - 1) {
                listStepCounter = listStepCooldown;
                AUDIO.Play(AudioType::POINT);
              }

              paStepIndex++;
            }
          }
        } 
      }
    }

    if (isBattleRoundOver && !isPlayerDeleted && player->GetHealth() > 0) {
      if (!battleResults) {
        sf::Time totalBattleTime = sf::milliseconds((sf::Int32)battleTimer.GetElapsed());

        // TODO: GetCounterCount()
        //       GetDoubleDelete()
        //       GetTripleDelete()
        battleResults = new BattleResults(totalBattleTime, player->GetMoveCount(), player->GetHitCount(), 0, false, false, mob);
        
        AUDIO.StopStream();
        AUDIO.Stream("resources/loops/enemy_deleted.ogg");
      }
      else {
        battleResults->Draw();

        if (!battleResults->IsInView()) {
          float amount = 600.f * elapsed;
          battleResults->Move(sf::Vector2f(amount, 0));
        }
        else {
          if (INPUT.has(PRESSED_A)) {
            // Have to hit twice
            if (battleResults->IsFinished()) {
              // TODO: sent the battle item off to the player's gaming session for storage
              BattleItem* reward = battleResults->GetReward();
              if (reward) delete reward;

              inBattleState = false;
            }
            else {
              AUDIO.Play(AudioType::ITEM_GET);
              battleResults->CursorAction();
            }
          }
        }
      }
    }
    else if (isBattleRoundOver && isPlayerDeleted) {
      if (!initFadeOut) {
        AUDIO.StopStream();
        shaderCooldown = 1;
        ENGINE.SetShader(&whiteShader);
        initFadeOut = true;
      }
      else {
        if (shaderCooldown < 0) {
          shaderCooldown = 0;
          inBattleState = false;
        }
      }

      shaderCooldown -= elapsed;

      whiteShader.setUniform("opacity", 1.f - (float)(shaderCooldown)*0.5f);
    }

    // Write contents to screen (always last step)
    ENGINE.Display();

    // TODO: make camera effects apply only to individual scenes that request them
    // This will avoid this hack here to move elements around on screen
    tile = nullptr;
    while (field->GetNextTile(tile)) {
      tile->move(cameraAntiOffset);
    }

    // update the cust if not paused nor in chip select nor in mob intro nor battle results
    if (!(isBattleRoundOver || isPaused || isInChipSelect || !mob->IsSpawningDone() || summons.IsSummonsActive() || isPreBattle)) {
      customProgress += elapsed;

      if (battleTimer.IsPaused()) {
        // start counting seconds again 
        battleTimer.Start();
      }
    }
    else {
      battleTimer.Pause();
    }

    if (customProgress / customDuration >= 1.0) {
      if (isChipSelectReady == false) {
        AUDIO.Play(AudioType::CUSTOM_BAR_FULL);
        isChipSelectReady = true;
      }
    } else {
      isChipSelectReady = false;
    }

    customBarShader.setUniform("factor", (float)(customProgress / customDuration));

    elapsed = static_cast<float>(clock.getElapsedTime().asSeconds());
  }

  delete folder;
  delete pauseLabel;
  delete font;
  delete mobFont;
  delete customBarTexture;
  delete background;

  if (battleResults) { delete battleResults; }

  AUDIO.StopStream();
  ENGINE.RevokeShader();

  return !isPlayerDeleted;
}