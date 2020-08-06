#include <Swoosh/ActivityController.h>
#include <chrono>

#include "bnNetworkBattleScene.h"
#include "bnPlayerInputReplicator.h"
#include "bnPlayerNetworkState.h"
#include "bnPlayerNetworkProxy.h"

#include "../bnCardLibrary.h"
#include "../bnGameOverScene.h"
#include "../bnUndernetBackground.h"
#include "../bnWeatherBackground.h"
#include "../bnRobotBackground.h"
#include "../bnMedicalBackground.h"
#include "../bnACDCBackground.h"
#include "../bnMiscBackground.h"
#include "../bnSecretBackground.h"
#include "../bnJudgeTreeBackground.h"
#include "../bnPlayerHealthUI.h"
#include "../bnPaletteSwap.h"
#include "../bnWebClientMananger.h"
#include "../bnFadeInState.h"

// Android only headers
#include "../Android/bnTouchArea.h"

#include "../Segues/WhiteWashFade.h"
#include "../Segues/PixelateBlackWashFade.h"

// modals like card cust and battle reward slide in 12px per frame for 10 frames. 60 frames = 1 sec
// modal slide moves 120px in 1/6th of a second
// Per 1 second that is 6*120px in 6*1/6 of a sec = 720px in 1 sec
#define MODAL_SLIDE_PX_PER_SEC 720.0f

// Combos are counted if more than one enemy is hit within x frames
// The game is clocked to display 60 frames per second
// If x = 20 frames, then we want a combo hit threshold of 20/60 = 0.3 seconds
#define COMBO_HIT_THRESHOLD_SECONDS 20.0f/60.0f

using namespace swoosh::types;

auto BuildMob = []() {
  Mob* mob = new Mob(new Field(6, 3));
  mob->SetBackground(new SecretBackground());
  return mob;
};

NetworkBattleScene::NetworkBattleScene(
  swoosh::ActivityController& controller, Player* player, CardFolder* folder, PA& programAdvance, const NetPlayConfig& config) :
        BattleScene(controller, player, BuildMob(), folder, programAdvance) {
  networkCardUseListener = new NetworkCardUseListener(*this, *player);
  networkCardUseListener->Subscribe(this->cardUI);

  Poco::Net::SocketAddress sa(Poco::Net::IPAddress(), config.myPort); 
  client = Poco::Net::DatagramSocket(sa);
  //client.bind(sa, true);
  client.setBlocking(false);

  remoteAddress = Poco::Net::SocketAddress(config.remoteIP, config.remotePort);
  client.connect(remoteAddress);

  selectedNavi = config.myNavi;

  player->CreateComponent<PlayerInputReplicator>(player);

  remoteShineAnimation = Animation("resources/mobs/boss_shine.animation");
  remoteShineAnimation.Load();

  remoteShine = sf::Sprite(*LOAD_TEXTURE(MOB_BOSS_SHINE));
  remoteShine.setScale(2.f, 2.f);
}

NetworkBattleScene::~NetworkBattleScene()
{ 
  delete remotePlayer;
  delete remoteCardUsePublisher;
  delete networkCardUseListener;
  client.close();
}

void NetworkBattleScene::onUpdate(double elapsed) {
  BattleScene::elapsed = elapsed;

  shineAnimation.Update((float)elapsed, shine);
  remoteShineAnimation.Update((float)elapsed, remoteShine);
  comboInfoTimer.update(elapsed);
  battleStartTimer.update(elapsed);
  battleEndTimer.update(elapsed);
  multiDeleteTimer.update(elapsed);
  summonTimer.update(elapsed);
  battleTimer.update(elapsed);
  PAStartTimer.update(elapsed);

  bool playerFormChangeComplete = !isChangingForm || (isChangingForm && isLeavingFormChange);
  bool remoteFormChangeComplete = !remoteState.remoteIsFormChanging || (remoteState.remoteIsFormChanging && remoteState.remoteIsLeavingFormChange);
  bool formChangeStateIsOver = (playerFormChangeComplete && remoteFormChangeComplete);

  if (!isChangingForm && !remoteState.remoteIsFormChanging) {
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
  else if (!formChangeStateIsOver) {
    showSummonBackdrop = true;

    if (showSummonBackdropTimer < showSummonBackdropLength) {
      showSummonBackdropTimer += elapsed;
    }

    auto handlePlayerFormChange = [this]
    (Player* which, int& lastSelectedFormRef, int formValue, 
      bool& isAnimating, bool& isLeaving, Animation& shineAnimationRef, sf::Sprite& shineRef) {
      which->Reveal(); // If flickering, stablizes the sprite for the animation

      if (showSummonBackdropTimer >= showSummonBackdropLength) {
        if (!isAnimating && (lastSelectedFormRef != formValue)) {
          isAnimating = true;

          // Preserve the original child node states
          auto childNodes = which->GetChildNodes();

          // The list of nodes change when adding the shine overlay and new form overlay nodes
          std::shared_ptr<std::vector<SceneNode*>> originalChildNodes(new std::vector<SceneNode*>(childNodes.size()));
          std::shared_ptr<std::vector<bool>> childShaderUseStates(new std::vector<bool>(childNodes.size()));


          auto paletteSwap = which->GetFirstComponent<PaletteSwap>();

          if (paletteSwap) paletteSwap->Enable(false);

          auto pos = which->getPosition();
          shineRef.setPosition(pos.x + 16.0f, pos.y - which->GetHeight() / 4.0f);

          auto onTransform = [
            this, which, &isAnimating, &isLeaving,
              states = childShaderUseStates, originals = originalChildNodes,
              &lastSelectedFormRef, formValue]() {
            // The next form has a switch based on health
            // This way dying will cancel the form
            // TODO: make this a separate function that takes in form index or something...
            lastSelectedFormRef = which->GetHealth() == 0 ? -1 : formValue;
            which->ActivateFormAt(lastSelectedFormRef);
            AUDIO.Play(AudioType::SHINE);

            which->SetShader(SHADERS.GetShader(ShaderType::WHITE));

            // Activating the form will add NEW child nodes onto our character
            // TODO: There's got to be a more optimal search than this...
            for (auto child : which->GetChildNodes()) {
              auto it = std::find_if(originals->begin(), originals->end(), [child](auto in) {
                return (child == in);
                });

              if (it == originals->end()) {
                states->push_back(child->IsUsingParentShader());
                originals->push_back(child);
                child->EnableParentShader(true); // Add new overlays to this list and make them temporarily white as well
              }
            }
          };

          auto onFinish = [this, &isLeaving, &isAnimating, 
                          paletteSwap, states = childShaderUseStates, originals = originalChildNodes]() {
            isLeaving = true;
            if (paletteSwap) paletteSwap->Enable();

            unsigned idx = 0;
            for (auto child : *originals) {
              if (!child) {
                idx++; continue;
              }

              unsigned thisIDX = idx;
              bool enabled = (*states)[idx++];
              child->EnableParentShader(enabled);
              Logger::Logf("Enabling state for child #%i: %s", thisIDX, enabled ? "true" : "false");
            }
          };

          shineAnimationRef << "SHINE" << Animator::On(10, onTransform) << Animator::On(20, onFinish);
        }
      }
    };

    handlePlayerFormChange(player, lastSelectedForm, cardCustGUI.GetSelectedFormIndex(), 
      isAnimatingFormChange, isLeavingFormChange, shineAnimation, shine);

    handlePlayerFormChange(remotePlayer, remoteState.remoteLastFormSelect, remoteState.remoteFormSelect,
      remoteState.remoteIsAnimatingFormChange, remoteState.remoteIsLeavingFormChange, remoteShineAnimation, remoteShine);
  }
  else if (formChangeStateIsOver && showSummonBackdropTimer > 0.0f) {
    showSummonBackdropTimer -= elapsed * 0.5f;

    if (showSummonBackdropTimer <= 0.0f) {
      isChangingForm = false; //done
      showSummonBackdrop = false;
      isLeavingFormChange = false; // sanity check
      isAnimatingFormChange = false; // sanity check
      remoteState.remoteIsFormChanging = false; // done
      remoteState.remoteIsAnimatingFormChange = false; // sanity check
      remoteState.remoteIsLeavingFormChange = false; // sanity check

      // Case: only show BattleStart if not triggered due to death
      // More reasons to use real state management for the battle scene...
      if (player->GetHealth() > 0 && remotePlayer->GetHealth() > 0) {
        this->sendReadySignal();
        this->sendHPSignal(this->clientPrevHP); // the last tracked health value from our update loop
      }
    }
  }

  ProcessNewestComponents();

  // Update components
  for (auto c : components) {
    c->OnUpdate((float)elapsed);
  }

  cardUI.OnUpdate((float)elapsed);

  if (battleResults) {
    battleResults->Update(elapsed);
  }

  isBattleRoundOver = (isPlayerDeleted || remoteState.isRemotePlayerLoser);

  if (this->isSceneInFocus && !isBattleRoundOver) processIncomingPackets();

  // Check if entire mob is deleted
  if (remoteState.isRemotePlayerLoser && !isPlayerDeleted) {
    if (!isPostBattle && battleEndTimer.getElapsed().asSeconds() < postBattleLength) {
      // Show Enemy Deleted
      isPostBattle = true;
      battleEndTimer.reset();
      AUDIO.StopStream();
      AUDIO.Stream("resources/loops/enemy_deleted.ogg");
      player->ChangeState<PlayerIdleState>();
      remotePlayer->Remove();
      field->RequestBattleStop();
    }
    else if (!isBattleRoundOver && battleEndTimer.getElapsed().asSeconds() > postBattleLength) {
      isMobDeleted = true;
    }
  }

  camera.Update((float)elapsed);

  background->Update((float)elapsed);

  if (!handshakeComplete && this->isSceneInFocus) {
    // Try reaching out to someone...
    this->sendConnectSignal(this->selectedNavi);
  }

  if (remoteState.isRemoteConnected) {
    if (isClientReady && !remoteState.isRemoteReady) {
      this->sendReadySignal();
      player->ChangeState<PlayerIdleState>(); // prevent movement
    }

    if (!handshakeComplete) {
      this->sendHandshakeSignal();
    }
  }

  // Do not update when: in card select, during a summon sequence, showing Battle Start sign
  if (!(isInCardSelect || (isChangingForm || remoteState.remoteIsFormChanging)) && !isPreBattle) {
    if (prevSummonState) {
      field->ToggleTimeFreeze(true);
    }

    field->Update((float)elapsed);
  }

  if (player) {
    int clientNewHP = player->GetHealth();
    if (this->clientPrevHP != clientNewHP) {
      this->clientPrevHP = clientNewHP;
      this->sendHPSignal(this->clientPrevHP);
    }
  }

  // TODO: we desperately need states
  // update the cust if not in card select nor in mob intro nor battle results nor post battle
  if (!(isBattleRoundOver || isInCardSelect || showSummonBackdrop
           || isPreBattle || isPostBattle || isChangingForm || remoteState.remoteIsFormChanging) 
    && remoteState.isRemoteReady && isClientReady) {
    if (battleTimer.isPaused()) {
      // start counting seconds again
      if (battleTimer.isPaused()) {
        battleTimer.start();
        comboDeleteCounter = 0; // reset the combo
        Logger::Log("comboDeleteCounter reset");
      }
    }

    customProgress += elapsed;

    field->ToggleTimeFreeze(false);

    // See if HP is 0 when we were in a form
    if (player && player->GetHealth() == 0 && lastSelectedForm != -1 && !isChangingForm ) {
      // If we were in a form, replay the animation
      // going back to our base this time

      isChangingForm = true;
      showSummonBackdropTimer = 0;
      backdropOpacity = 1.0f; // full black
      this->sendChangedFormSignal(-1); // back to original
    }

    if (remotePlayer && remotePlayer->GetHealth() == 0 && remoteState.remoteFormSelect != -1 && !remoteState.remoteIsFormChanging) {
      // same but for remote
      remoteState.remoteIsFormChanging = true;
      showSummonBackdropTimer = 0;
      backdropOpacity = 1.0f; // full black
    }
  }
  else {
    battleTimer.pause();
  }

  cardCustGUI.Update((float)elapsed);
}

void NetworkBattleScene::onDraw(sf::RenderTexture& surface) {
  ENGINE.SetRenderSurface(surface);

  ENGINE.Clear();

  if (isChangingForm || remoteState.remoteIsFormChanging) {
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
      SpriteProxyNode* coloredTile = new SpriteProxyNode(*(sf::Sprite*)tile);
      coloredTile->SetShader(&pauseShader);
      pauseShader.setUniform("opacity", (float)backdropOpacity*float(std::max(0.0, (showSummonBackdropTimer / showSummonBackdropLength))));
      ENGINE.Draw(coloredTile);
      delete coloredTile;
    }
    else {
      if (tile->IsHighlighted()) {
        SpriteProxyNode* coloredTile = new SpriteProxyNode(*(sf::Sprite*)tile);
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
        if (!entity->WillRemoveLater()) {
          auto uic = entity->GetComponentsDerivedFrom<UIComponent>();

          //Logger::Log("uic size is: " + std::to_string(uic.size()));

          if (!uic.empty()) {
            ui.insert(ui.begin(), uic.begin(), uic.end());
          }

          entitiesOnRow.push_back(*entitiesIter);

        }

          entitiesIter++;
      }

      /*if (tile->GetState() == TileState::lava) {
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
      else if (tile->GetState() == TileState::ice) {
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

  if (remoteState.remoteIsAnimatingFormChange) {
    surface.draw(remoteShine);
  }

  // cust dissapears when not in battle
  if ((!(isInCardSelect || isPostBattle) && remoteState.isRemoteConnected))
    ENGINE.Draw(&customBarSprite);

  // TODO: hack to swap out
  /*static auto lastShader = player->GetShader();

  if (!isInCardSelect && showSummonBackdropTimer > showSummonBackdropLength/25.0f) {
    player->SetShader(SHADERS.GetShader(ShaderType::WHITE));
  }
  else {
    player->SetShader(lastShader);
  }*/

  if (!summons.IsSummonActive() && showSummonText) {
    sf::Text summonsLabel = sf::Text(summons.GetSummonLabel(), *mobFont);

    double summonSecs = summonTimer.getElapsed().asSeconds() - showSummonBackdropLength;
    double scale = swoosh::ease::wideParabola(summonSecs, summonTextLength, 3.0);

    if (summons.GetCallerTeam() == Team::red) {
      summonsLabel.setPosition(40.0f, 80.f);
    }
    else {
      summonsLabel.setPosition(470.0f, 80.0f);
    }

    summonsLabel.setScale(1.0f, (float)scale);
    summonsLabel.setOutlineColor(sf::Color::Black);
    summonsLabel.setFillColor(sf::Color::White);
    summonsLabel.setOutlineThickness(2.f);

    if (summons.GetCallerTeam() == Team::red) {
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
  if (isInCardSelect) {
    //for (int i = 0; i < mob->GetMobCount(); i++) {
    //  if (mob->GetMobAt(i).IsDeleted())
    //    continue;

      sf::Text mobLabel = sf::Text(remotePlayer->GetName(), *mobFont);

      mobLabel.setOrigin(mobLabel.getLocalBounds().width, 0);
      mobLabel.setPosition(470.0f, -1.f + nextLabelHeight);
      mobLabel.setScale(0.8f, 0.8f);
      mobLabel.setOutlineColor(sf::Color(48, 56, 80));
      mobLabel.setOutlineThickness(2.f);
      ENGINE.Draw(mobLabel, false);

        // make the next label relative to this one
   //   nextLabelHeight += mobLabel.getLocalBounds().height;
   // }
  }


  if (!isPlayerDeleted && !showSummonBackdrop && !summons.IsSummonActive()) {
    ENGINE.Draw(cardUI);
  }

  if (isPreBattle && !isChangingForm && !remoteState.remoteIsFormChanging) {
    if (preBattleLength <= 0) {
      isPreBattle = false;
    }
    else {
      double battleStartSecs = battleStartTimer.getElapsed().asSeconds();
      double scale = swoosh::ease::wideParabola(battleStartSecs, preBattleLength, 2.0);
      battleStart.setScale(2.f, (float)scale*2.f);

      if (battleStartSecs >= preBattleLength) {
        isPreBattle = false;
        player->ChangeState<PlayerControlledState>();
        remotePlayer->ChangeState<PlayerNetworkState>(remoteState);
      }

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

  // compare the summon state after we used a card...
  if (!showSummonText) {
    if (summons.IsSummonOver() && prevSummonState == true) {
      // We are leaving the summons state this frame
      summons.OnLeave();
      prevSummonState = false;
    }
    else   // When these conditions are met, the card name has shown and we're ready to follow through with the summon
    if (summons.IsSummonActive() && showSummonBackdrop) {
      summons.Update(elapsed);
    }

    // Track if a summon card was used on this frame
    if (!prevSummonState) {
      prevSummonState = summons.HasMoreInQueue() && player->GetFirstComponent<AnimationComponent>()->GetAnimationString() == "PLAYER_IDLE";

      if (prevSummonState) {
        summonTimer.reset();
        showSummonBackdrop = true;
        showSummonBackdropTimer = 0;
      }
    }
  }


  // Draw cust GUI on top of scene. No shaders affecting.
  ENGINE.Draw(cardCustGUI);

  // Scene keyboard controls
  // TODO: really belongs in Update() but also handles a lot of conditional draws
  //       refactoring battle scene into battle states should reduce this complexity
  if (
           (
                   customProgress >= customDuration && !isInCardSelect && handshakeComplete &&
                   !isBattleRoundOver && summons.IsSummonOver() && !isPreBattle && !isPostBattle
           )) {
    static bool once = true;

    if (once) {
      field->ToggleTimeFreeze(true);
      // show the card select screen
      customProgress = customDuration;
      field->RequestBattleStart();
      once = false;
    }
    else if (isInCardSelect == false && !isBattleRoundOver) {
      player->SetCharging(false);
      remotePlayer->SetCharging(false);

      AUDIO.Play(AudioType::CUSTOM_SCREEN_OPEN);
      // slide up the screen a hair
      //camera.MoveCamera(sf::Vector2f(240.f, 140.f), sf::seconds(0.5f));
      isInCardSelect = true;

      // Clear any card UI queues. they will contain null data.
      cardUI.LoadCards(0, 0);

      // Reset PA system
      isPAComplete = false;
      hasPA = -1;
      paStepIndex = 0;
      listStepCounter = listStepCooldown;

      // Load the next cards
      cardCustGUI.ResetState();
      cardCustGUI.GetNextCards();

      this->isClientReady = this->remoteState.isRemoteReady = false;;

      // resync health real quick before everything pauses
      this->sendHPSignal(player->GetHealth());
    }

    // NOTE: Need a battle scene state manager to handle going to and from one controll scheme to another.
    //       This has become unweidly.
    //       Plus would make more sense to revoke screen effects and labels once 
    //       a state has terminated

  }
  else if (isInCardSelect && cardCustGUI.IsInView()) {
#ifndef __ANDROID__
    if (INPUTx.Has(EventTypes::PRESSED_SCAN_RIGHT) || INPUTx.Has(EventTypes::PRESSED_SCAN_LEFT)) {
        if (cardCustGUI.IsVisible()) {
            cardCustGUI.Hide();
        }
        else {
            cardCustGUI.Reveal();
        }
    } 
    
    if (cardCustGUI.CanInteract()) {
        if (cardCustGUI.IsCardDescriptionTextBoxOpen()) {
            if (!INPUTx.Has(EventTypes::HELD_QUICK_OPT)) {
                cardCustGUI.CloseCardDescription() ? AUDIO.Play(AudioType::CHIP_DESC_CLOSE, AudioPriority::lowest) : 1;
            }
            else if (INPUTx.Has(EventTypes::PRESSED_CONFIRM)) {

                cardCustGUI.CardDescriptionConfirmQuestion() ? AUDIO.Play(AudioType::CHIP_CHOOSE) : 1;
                cardCustGUI.ContinueCardDescription();
            }

            cardCustGUI.FastForwardCardDescription(4.0);

            if (INPUTx.Has(EventTypes::PRESSED_UI_LEFT)) {
              cardCustGUI.CardDescriptionYes(); //? AUDIO.Play(AudioType::CHIP_SELECT) : 1;;
            }
            else if (INPUTx.Has(EventTypes::PRESSED_UI_RIGHT)) {
              cardCustGUI.CardDescriptionNo(); //? AUDIO.Play(AudioType::CHIP_SELECT) : 1;;
            }
        }
        else {
          // there's a wait time between moving ones and moving repeatedly (Sticky keys)
          bool moveCursor = cardSelectInputCooldown <= 0 || cardSelectInputCooldown == heldCardSelectInputCooldown;

            if (INPUTx.Has(EventTypes::PRESSED_UI_LEFT) || INPUTx.Has(EventTypes::HELD_UI_LEFT)) {
                cardSelectInputCooldown -= elapsed;

                if (moveCursor) {
                    cardCustGUI.CursorLeft() ? AUDIO.Play(AudioType::CHIP_SELECT) : 1;

                    if (cardSelectInputCooldown <= 0) {
                      cardSelectInputCooldown = maxCardSelectInputCooldown; // sticky key time
                    }
                }
            }
            else if (INPUTx.Has(EventTypes::PRESSED_UI_RIGHT) || INPUTx.Has(EventTypes::HELD_UI_RIGHT)) {
                cardSelectInputCooldown -= elapsed;

                if (moveCursor) {
                    cardCustGUI.CursorRight() ? AUDIO.Play(AudioType::CHIP_SELECT) : 1;

                    if (cardSelectInputCooldown <= 0) {
                      cardSelectInputCooldown = maxCardSelectInputCooldown; // sticky key time
                    }
                }
            }
            else if (INPUTx.Has(EventTypes::PRESSED_UI_UP) ||  INPUTx.Has(EventTypes::HELD_UI_UP)) {
                cardSelectInputCooldown -= elapsed;

                if (moveCursor) {
                    cardCustGUI.CursorUp() ? AUDIO.Play(AudioType::CHIP_SELECT) : 1;

                    if (cardSelectInputCooldown <= 0) {
                      cardSelectInputCooldown = maxCardSelectInputCooldown; // sticky key time
                    }
                }
            }
            else if (INPUTx.Has(EventTypes::PRESSED_UI_DOWN) ||  INPUTx.Has(EventTypes::HELD_UI_DOWN)) {
                cardSelectInputCooldown -= elapsed;

                if (moveCursor) {
                    cardCustGUI.CursorDown() ? AUDIO.Play(AudioType::CHIP_SELECT) : 1;

                    if (cardSelectInputCooldown <= 0) {
                      cardSelectInputCooldown = maxCardSelectInputCooldown; // sticky key time
                    }
                }
            }
            else {
                cardSelectInputCooldown = heldCardSelectInputCooldown;
            }

            if (INPUTx.Has(EventTypes::PRESSED_CONFIRM)) {
                bool performed = cardCustGUI.CursorAction();

                if (cardCustGUI.AreCardsReady()) {
                    AUDIO.Play(AudioType::CHIP_CONFIRM, AudioPriority::high);
                    customProgress = 0; // NOTE: Temporary Hack. We base the cust state around the custom Progress value.
                    //camera.MoveCamera(sf::Vector2f(240.f, 160.f), sf::seconds(0.5f));
                }
                else if (performed) {
                    if (!cardCustGUI.SelectedNewForm()) {
                        AUDIO.Play(AudioType::CHIP_CHOOSE, AudioPriority::highest);
                    }
                }
                else {
                    AUDIO.Play(AudioType::CHIP_ERROR, AudioPriority::lowest);
                }
            }
            else if (INPUTx.Has(EventTypes::PRESSED_CANCEL) || sf::Mouse::isButtonPressed(sf::Mouse::Button::Right)) {
                cardCustGUI.CursorCancel() ? AUDIO.Play(AudioType::CHIP_CANCEL, AudioPriority::highest) : 1;
            }
            else if (INPUTx.Has(EventTypes::HELD_QUICK_OPT)) {
                cardCustGUI.OpenCardDescription() ? AUDIO.Play(AudioType::CHIP_DESC, AudioPriority::lowest) : 1;
            }
            else if (INPUTx.Has(EventTypes::HELD_PAUSE)) {
              cardCustGUI.CursorSelectOK() ? AUDIO.Play(AudioType::CHIP_CANCEL, AudioPriority::lowest) : 1;;
            }
        }
    }

#else
    static bool isHidden = false;

    if(INPUTx.Has(EventTypes::RELEASED_LEFT)) {
      if(!isHidden) {
        cardCustGUI.Hide();
        isHidden = true;
      } else {
        cardCustGUI.Reveal();
        isHidden = false;
      }
    }

    if (cardCustGUI.AreCardsReady() && !isHidden) {
      AUDIO.Play(AudioType::CHIP_CONFIRM, AudioPriority::high);
      customProgress = 0; // NOTE: Temporary Hack. We base the cust state around the custom Progress value.
    }
#endif
  }

  if (isInCardSelect && customProgress > 0.f) {
    if (!cardCustGUI.IsInView()) {
      cardCustGUI.Move(sf::Vector2f(MODAL_SLIDE_PX_PER_SEC * (float)elapsed, 0));
    }
  }
  else {
    if (!cardCustGUI.IsOutOfView()) {
      cardCustGUI.Move(sf::Vector2f(-MODAL_SLIDE_PX_PER_SEC * (float)elapsed, 0));
    }
    else if (isInCardSelect) { // we're leaving a state
      // Start Program Advance checks
      if (isPAComplete && hasPA == -1) {
        // Filter and apply support cards
        FilterSupportCards(cards, cardCount);

        // Return to game
        isInCardSelect = false;
        cardUI.LoadCards(cards, cardCount);
        ENGINE.RevokeShader();

        int selectedForm = cardCustGUI.GetSelectedFormIndex();

        if (selectedForm != lastSelectedForm) {
          isChangingForm = true;
          showSummonBackdropTimer = 0;
          backdropOpacity = 1.0f; // full black
          this->sendChangedFormSignal(selectedForm);
        }
        else {
          this->sendReadySignal();
        }
      }
      else if (!isPAComplete) {
        cards = cardCustGUI.GetCards();
        cardCount = cardCustGUI.GetCardCount();

        hasPA = programAdvance.FindPA(cards, cardCount);

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

        if (paStepIndex <= cardCount + 1) {
          for (int i = 0; i < paStepIndex && i < cardCount; i++) {
            std::string formatted = cards[i]->GetShortName();
            formatted.resize(9, ' ');
            formatted[8] = cards[i]->GetCode();

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

          for (int i = 0; i < cardCount; i++) {
            std::string formatted = cards[i]->GetShortName();
            formatted.resize(9, ' ');
            formatted[8] = cards[i]->GetCode();

            sf::Text stepLabel = sf::Text(formatted, *mobFont);

            stepLabel.setOrigin(0, 0);
            stepLabel.setPosition(40.0f, 80.f + (nextLabelHeight*2.f));
            stepLabel.setScale(1.0f, 1.0f);
            stepLabel.setOutlineColor(sf::Color(48, 56, 80));
            stepLabel.setOutlineThickness(2.f);

            if (i >= hasPA && i <= hasPA + paSteps.size() - 1) {
              if (i == hasPA) {
                Battle::Card* paCard = programAdvance.GetAdvanceCard();

                sf::Text stepLabel = sf::Text(paCard->GetShortName(), *mobFont);
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
          // +2 = 1 step for showing PA label and 1 step for showing merged card
          // That's the cards we want to show + 1 + 1 = cardCount + 2
          if (paStepIndex == cardCount + 2) {
            advanceSoundPlay = false;

            Battle::Card* paCard = programAdvance.GetAdvanceCard();

            // Only remove the cards involved in the program advance. Replace them with the new PA card.
            // PA card is dealloc by the class that created it so it must be removed before the library tries to dealloc
            int newCardCount = cardCount - (int)paSteps.size() + 1; // Add the new one
            int newCardStart = hasPA;

            // Create a temp card list
            Battle::Card** newCardList = new Battle::Card*[newCardCount];

            int j = 0;
            for (int i = 0; i < cardCount; ) {
              if (i == hasPA) {
                newCardList[j] = paCard;
                i += (int)paSteps.size();
                j++;
                continue;
              }

              newCardList[j] = cards[i];
              i++;
              j++;
            }

            // Set the new cards
            for (int i = 0; i < newCardCount; i++) {
              cards[i] = *(newCardList + i);
            }

            // Delete the temp list space
            // NOTE: We are _not_ deleting the pointers in them
            delete[] newCardList;

            cardCount = newCardCount;

            hasPA = -1; // state over
          }
          else {
            if (paStepIndex == cardCount + 1) {
              listStepCounter = listStepCooldown * 2.0f; // Linger on the screen when showing the final PA
            }
            else {
              listStepCounter = listStepCooldown * 0.7f; // Quicker about non-PA cards
            }

            if (paStepIndex >= hasPA && paStepIndex <= hasPA + paSteps.size() - 1) {
              listStepCounter = listStepCooldown; // Take our time with the PA cards
              AUDIO.Play(AudioType::POINT_SFX);
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
        if (INPUTx.Has(EventTypes::PRESSED_CONFIRM)) {
          // Have to hit twice
          if (battleResults->IsFinished()) {
            BattleItem* reward = battleResults->GetReward();

            if (reward != nullptr) {
              if (reward->IsCard()) {
                // TODO: send the battle item off to the player's
                // persistent session storage (aka a save file or cloud database)
                CHIPLIB.AddCard(reward->GetCard());
                delete reward;
              }
            }

            using effect = segue<PixelateBlackWashFade, milliseconds<500>>;
            getController().queuePop<effect>();
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
      using effect = segue<WhiteWashFade, milliseconds<500>>;
      getController().queueRewind<effect::to<GameOverScene>>();
      this->sendLoserSignal();
    }
  }

  if (customProgress / customDuration >= 1.0) {
    if (isCardSelectReady == false) {
      AUDIO.Play(AudioType::CUSTOM_BAR_FULL);
      isCardSelectReady = true;
    }
  }
  else {
    isCardSelectReady = false;
  }

  customBarShader.setUniform("factor", (float)(customProgress / customDuration));
}

void NetworkBattleScene::Inject(PlayerInputReplicator& pub)
{
}

void NetworkBattleScene::sendHandshakeSignal()
{
  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::handshake };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  client.sendBytes(buffer.begin(), buffer.size());
}

void NetworkBattleScene::sendShootSignal()
{
  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::shoot };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  client.sendBytes(buffer.begin(), buffer.size());
}

void NetworkBattleScene::sendUseSpecialSignal()
{
  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::special };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  client.sendBytes(buffer.begin(), buffer.size());
}

void NetworkBattleScene::sendChargeSignal(const bool state)
{
  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::charge };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  buffer.append((char*)&state, sizeof(bool));
  client.sendBytes(buffer.begin(), buffer.size());
}

void NetworkBattleScene::sendConnectSignal(const SelectedNavi navi)
{
  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::connect };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  buffer.append((char*)&navi, sizeof(SelectedNavi));
  client.sendBytes(buffer.begin(), buffer.size());
}

void NetworkBattleScene::sendReadySignal()
{
  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::ready };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  client.sendBytes(buffer.begin(), buffer.size());

  if (remoteState.isRemoteReady) {
    this->isPreBattle = true;
    this->battleStartTimer.reset();
  }

  this->isClientReady = true;
}

void NetworkBattleScene::sendChangedFormSignal(const int form)
{
  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::form };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  buffer.append((char*)&form, sizeof(int));
  client.sendBytes(buffer.begin(), buffer.size());
}

void NetworkBattleScene::sendMoveSignal(const Direction dir)
{
  Logger::Logf("sending dir of %i", dir);

  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::move };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  buffer.append((char*)&dir, sizeof(char));
  client.sendBytes(buffer.begin(), buffer.size());
}

void NetworkBattleScene::sendHPSignal(const int hp)
{
  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::hp };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  buffer.append((char*)&hp, sizeof(int));
  client.sendBytes(buffer.begin(), buffer.size());
}

void NetworkBattleScene::sendTileCoordSignal(const int x, const int y)
{
  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::tile };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  buffer.append((char*)&x, sizeof(int));
  buffer.append((char*)&y, sizeof(int));
  client.sendBytes(buffer.begin(), buffer.size());
}

void NetworkBattleScene::sendChipUseSignal(const std::string& used)
{
  Logger::Logf("sending chip data over network for %s", used.data());

  uint64_t timestamp = (uint64_t)CurrentTime::AsMilli();

  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::chip };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  buffer.append((char*)&timestamp, sizeof(uint64_t));
  buffer.append((char*)used.data(),used.length());
  client.sendBytes(buffer.begin(), buffer.size());
}

void NetworkBattleScene::sendLoserSignal()
{
  Poco::Buffer<char> buffer{ 0 };
  NetPlaySignals type{ NetPlaySignals::loser };
  buffer.append((char*)&type, sizeof(NetPlaySignals));
  client.sendBytes(buffer.begin(), buffer.size());
}

void NetworkBattleScene::recieveHandshakeSignal()
{
  this->handshakeComplete = true;
  this->sendHandshakeSignal();
}

void NetworkBattleScene::recieveShootSignal()
{
  if (!remoteState.isRemoteConnected) return;

  //remotePlayer->Attack();
  remoteState.remoteShoot = true;
  Logger::Logf("recieved shoot signal from remote");
}

void NetworkBattleScene::recieveUseSpecialSignal()
{
  if (!remoteState.isRemoteConnected) return;

  // remotePlayer->UseSpecial();
  remoteState.remoteUseSpecial = true;

  Logger::Logf("recieved use special signal from remote");

}

void NetworkBattleScene::recieveChargeSignal(const Poco::Buffer<char>& buffer)
{
  if (!remoteState.isRemoteConnected) return;

  bool state = remoteState.remoteCharge; std::memcpy(&state, buffer.begin(), buffer.size());
  remoteState.remoteCharge = state;

  Logger::Logf("recieved charge signal from remote: %i", state);

}

void NetworkBattleScene::recieveConnectSignal(const Poco::Buffer<char>& buffer)
{
  if (remoteState.isRemoteConnected) return; // prevent multiple connection requests...

  remoteState.isRemoteConnected = true;

  SelectedNavi navi = SelectedNavi{ 0 }; std::memcpy(&navi, buffer.begin(), buffer.size());
  remoteState.remoteNavi = navi;

  Logger::Logf("Recieved connect signal! Remote navi: %i", remoteState.remoteNavi);

  assert(remotePlayer == nullptr && "remote player was already set!");
  remotePlayer = NAVIS.At(navi).GetNavi();
  remotePlayer->SetTeam(Team::blue);
  remotePlayer->setScale(remotePlayer->getScale().x * -1.0f, remotePlayer->getScale().y);
  remotePlayer->ChangeState<PlayerIdleState>();
  remotePlayer->Character::Update(0);

  FinishNotifier onFinish = [this]() {
    customProgress = customDuration; // open the cust
  };

  remotePlayer->ChangeState<FadeInState<Player>>(onFinish);
  field->AddEntity(*remotePlayer, remoteState.remoteTileX, remoteState.remoteTileY);
  remotePlayer->ToggleTimeFreeze(false);

  remoteState.remoteHP = remotePlayer->GetHealth();

  remoteCardUsePublisher = remotePlayer->CreateComponent<SelectedCardsUI>(remotePlayer);
  remoteCardUseListener = new PlayerCardUseListener(*remotePlayer);
  remoteCardUseListener->Subscribe(*remoteCardUsePublisher);
  this->summons.Subscribe(*remoteCardUsePublisher);

  remotePlayer->CreateComponent<MobHealthUI>(remotePlayer);
  remotePlayer->CreateComponent<PlayerNetworkProxy>(remotePlayer, remoteState);
}

void NetworkBattleScene::recieveReadySignal()
{
  if (!remoteState.isRemoteConnected) return;

  if (this->isClientReady && !remoteState.isRemoteReady) {
    this->isPreBattle = true;
    this->battleStartTimer.reset();
  }

  remoteState.isRemoteReady = true;
}

void NetworkBattleScene::recieveChangedFormSignal(const Poco::Buffer<char>& buffer)
{
  if (!remoteState.isRemoteConnected) return;

  int form = remoteState.remoteFormSelect;
  int prevForm = remoteState.remoteFormSelect;
  std::memcpy(&form, buffer.begin(), sizeof(int));

  if (remotePlayer && form != prevForm) {
    // If we were in a form, replay the animation
    // going back to our base this time
    remoteState.remoteLastFormSelect = remoteState.remoteFormSelect;
    remoteState.remoteFormSelect = form;
    remoteState.remoteIsFormChanging = true; // begins the routine
    remoteState.remoteIsAnimatingFormChange = false; // state moment flags must be false to trigger
    remoteState.remoteIsLeavingFormChange = false;   // ... after the backdrop turns dark
    showSummonBackdropTimer = 0;
    backdropOpacity = 1.0f; // full black
  }
}

void NetworkBattleScene::recieveMoveSignal(const Poco::Buffer<char>& buffer)
{
  if (!remoteState.isRemoteConnected) return;

  Direction dir = remoteState.remoteDirection; std::memcpy(&dir, buffer.begin(), buffer.size());

  if (!player->Teammate(remotePlayer->GetTeam())) {
    if (dir == Direction::left || dir == Direction::right) {
      dir = Reverse(dir);
    }
  }

  Logger::Logf("recieved move signal from remote %i", dir);

  remoteState.remoteDirection = dir;
}

void NetworkBattleScene::recieveHPSignal(const Poco::Buffer<char>& buffer)
{
  if (!remoteState.isRemoteConnected) return;

  int hp = remoteState.remoteHP; std::memcpy(&hp, buffer.begin(), buffer.size());
  remoteState.remoteHP = hp;
  remotePlayer->SetHealth(hp);
}

void NetworkBattleScene::recieveTileCoordSignal(const Poco::Buffer<char>& buffer)
{
  if (!remoteState.isRemoteConnected) return;

  int x = remoteState.remoteTileX; std::memcpy(&x, buffer.begin(), sizeof(int));
  int y = remoteState.remoteTileX; std::memcpy(&y, (buffer.begin()+sizeof(int)), sizeof(int));

  // mirror the x value for remote
  x = (field->GetWidth() - x)+1;

  remoteState.remoteTileX = x;
  remoteState.remoteTileY = y;

  Battle::Tile* t = field->GetAt(x, y);

  if (remotePlayer->GetTile() != t && !remotePlayer->IsSliding()) {
    remotePlayer->GetTile()->RemoveEntityByID(remotePlayer->GetID());
    remotePlayer->AdoptTile(t);
    remotePlayer->FinishMove();
  }
}

void NetworkBattleScene::recieveChipUseSignal(const Poco::Buffer<char>& buffer)
{
  if (!remoteState.isRemoteConnected) return;

  uint64_t timestamp = 0; std::memcpy(&timestamp, buffer.begin(), sizeof(uint64_t));
  std::string used = std::string(buffer.begin()+sizeof(uint64_t), buffer.size()-sizeof(uint64_t));
  remoteState.remoteChipUse = used;
  Battle::Card card = WEBCLIENT.MakeBattleCardFromWebCardData(WebAccounts::Card{ used });
  remoteCardUsePublisher->Broadcast(card, *remotePlayer, timestamp);
  Logger::Logf("remote used chip %s", used.c_str());
}

void NetworkBattleScene::recieveLoserSignal()
{
  remoteState.isRemotePlayerLoser = true;
}

void NetworkBattleScene::processIncomingPackets()
{
  if (!client.poll(Poco::Timespan{ 0 }, Poco::Net::Socket::SELECT_READ)) return;

  static int errorCount = 0;

  if (errorCount > 10) {
    AUDIO.StopStream();
    using effect = segue<PixelateBlackWashFade>;
    getController().queuePop<effect>();
    errorCount = 0; // reset for next match
    return;
  }

  static char rawBuffer[MAX_BUFFER_LEN] = { 0 };
  static int read = 0;

  try {
    read+= client.receiveBytes(rawBuffer, MAX_BUFFER_LEN-1);
    if (read > 0) {
      rawBuffer[read] = '\0';

      NetPlaySignals sig = *(NetPlaySignals*)rawBuffer;
      size_t sigLen = sizeof(NetPlaySignals);
      Poco::Buffer<char> data{ 0 };
      data.append(rawBuffer + sigLen, size_t(read)-sigLen);

      switch (sig) {
      case NetPlaySignals::handshake:
        recieveHandshakeSignal();
        break;
      case NetPlaySignals::connect:
        recieveConnectSignal(data);
        break;
      case NetPlaySignals::chip:
        recieveChipUseSignal(data);
        break;
      case NetPlaySignals::form:
        recieveChangedFormSignal(data);
        break;
      case NetPlaySignals::hp:
        recieveHPSignal(data);
        break;
      case NetPlaySignals::loser:
        recieveLoserSignal();
        break;
      case NetPlaySignals::move:
        recieveMoveSignal(data);
        break;
      case NetPlaySignals::ready:
        recieveReadySignal();
        break;
      case NetPlaySignals::tile:
        recieveTileCoordSignal(data);
        break;
      case NetPlaySignals::shoot:
        recieveShootSignal();
        break;
      case NetPlaySignals::special:
        recieveUseSpecialSignal();
        break;
      case NetPlaySignals::charge:
        recieveChargeSignal(data);
        break;
      }
    }

    errorCount = 0;
  }
  catch (std::exception& e) {
    std::cout << "Network exception: " << e.what() << std::endl;
    
    if (remoteState.isRemoteConnected) {
      errorCount++;
    }
  }

  read = 0;
  std::memset(rawBuffer, 0, MAX_BUFFER_LEN);
}
