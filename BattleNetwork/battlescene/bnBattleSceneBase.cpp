#include "bnBattleSceneBase.h"

#include "../bnTextureResourceManager.h"
#include "../bnShaderResourceManager.h"
#include "../bnMob.h"
#include "../bnPlayerHealthUI.h"
#include "../bnUndernetBackground.h"
#include "../bnWeatherBackground.h"
#include "../bnRobotBackground.h"
#include "../bnMedicalBackground.h"
#include "../bnACDCBackground.h"
#include "../bnMiscBackground.h"
#include "../bnSecretBackground.h"
#include "../bnJudgeTreeBackground.h"
#include "../bnLanBackground.h"
#include "../bnGraveyardBackground.h"
#include "../bnVirusBackground.h"

// Combos are counted if more than one enemy is hit within x frames
// The game is clocked to display 60 frames per second
// If x = 20 frames, then we want a combo hit threshold of 20/60 = 0.3 seconds
#define COMBO_HIT_THRESHOLD_SECONDS 20.0f/60.0f

using swoosh::types::segue;
using swoosh::Activity;
using swoosh::ActivityController;

BattleSceneBase::BattleSceneBase(ActivityController& controller, const BattleSceneBaseProps& props) : Activity(&controller),
  player(&props.player),
  programAdvance(props.programAdvance),
  comboDeleteCounter(0),
  totalCounterMoves(0),
  totalCounterDeletions(0),
  whiteShader(*SHADERS.GetShader(ShaderType::WHITE_FADE)),
  yellowShader(*SHADERS.GetShader(ShaderType::YELLOW)),
  heatShader(*SHADERS.GetShader(ShaderType::SPOT_DISTORTION)),
  iceShader(*SHADERS.GetShader(ShaderType::SPOT_REFLECTION)),
  distortionMap(*TEXTURES.GetTexture(TextureType::HEAT_TEXTURE)),
  cardListener(props.player),
  // cap of 8 cards, 8 cards drawn per turn
  cardCustGUI(props.folder, 8, 8),
  camera(*ENGINE.GetCamera()),
  cardUI(&props.player) {

  /*
  Set Scene*/
  field = props.field;
  CharacterDeleteListener::Subscribe(*field);

  player->ChangeState<PlayerIdleState>();
  player->ToggleTimeFreeze(false);
  field->AddEntity(*player, 2, 2);

  // Card UI for player
  cardListener.Subscribe(cardUI);
  this->CardUseListener::Subscribe(cardUI);

  /*
  Background for scene*/
  background = props.background;

  if (!background) {
    int randBG = rand() % 10;

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
    else if (randBG == 9) {
      background = new SecretBackground();
    }
  }

  PlayerHealthUI* healthUI = new PlayerHealthUI(player);
  cardCustGUI.AddNode(healthUI);
  components.push_back((UIComponent*)healthUI);

  camera = Camera(ENGINE.GetView());
  ENGINE.SetCamera(camera);

  /*
  Other battle labels
  */

  doubleDelete = sf::Sprite(*LOAD_TEXTURE(DOUBLE_DELETE));
  doubleDelete.setOrigin(doubleDelete.getLocalBounds().width / 2.0f, doubleDelete.getLocalBounds().height / 2.0f);
  comboInfoPos = sf::Vector2f(240.0f, 50.f);
  doubleDelete.setPosition(comboInfoPos);
  doubleDelete.setScale(2.f, 2.f);

  tripleDelete = doubleDelete;
  tripleDelete.setTexture(*LOAD_TEXTURE(TRIPLE_DELETE));

  counterHit = doubleDelete;
  counterHit.setTexture(*LOAD_TEXTURE(COUNTER_HIT));

  counterRevealAnim = Animation("resources/navis/counter_reveal.animation");
  counterRevealAnim << "DEFAULT" << Animator::Mode::Loop;

  counterReveal.setTexture(LOAD_TEXTURE(MISC_COUNTER_REVEAL), true);
  counterReveal.EnableParentShader(false);
  counterReveal.SetLayer(-100);

  counterCombatRule = new CounterCombatRule(this);

  /*
  Cards + Card select setup*/
  cards = nullptr;
  cardCount = 0;

  // PAUSE
  font = TEXTURES.LoadFontFromFile("resources/fonts/dr_cain_terminal.ttf");
  pauseLabel = new sf::Text("paused", *font);
  pauseLabel->setOrigin(pauseLabel->getLocalBounds().width / 2, pauseLabel->getLocalBounds().height * 2);
  pauseLabel->setPosition(sf::Vector2f(240.f, 160.f));

  // Load forms
  cardCustGUI.SetPlayerFormOptions(player->GetForms());

  // MOB UI
  mobFont = TEXTURES.LoadFontFromFile("resources/fonts/mmbnthick_regular.ttf");
  mobBackdropSprite = sf::Sprite(*LOAD_TEXTURE(MOB_NAME_BACKDROP));
  mobEdgeSprite = sf::Sprite(*LOAD_TEXTURE(MOB_NAME_EDGE));

  mobBackdropSprite.setScale(2.f, 2.f);
  mobEdgeSprite.setScale(2.f, 2.f);

  customProgress = 0; // in seconds
  customDuration = 10; // 10 seconds

  backdropOpacity = 0.25; // default is 25%

  // SHADERS
  // TODO: Load shaders if supported
  shaderCooldown = 0;

  whiteShader.setUniform("texture", sf::Shader::CurrentTexture);
  whiteShader.setUniform("opacity", 0.5f);
  whiteShader.setUniform("texture", sf::Shader::CurrentTexture);

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

  isSceneInFocus = false;
}

BattleSceneBase::~BattleSceneBase() {
  for (auto elem : nodeToEdges) {
    delete elem.second;
  }

  for (auto elem : states) {
    delete elem;
  }
}

const bool BattleSceneBase::DoubleDelete() const
{
  return didDoubleDelete;
}
const bool BattleSceneBase::TripleDelete() const
{
  return didTripleDelete;
}
;

const int BattleSceneBase::GetCounterCount() const {
  return totalCounterMoves;
}

const bool BattleSceneBase::IsSceneInFocus() const
{
  return isSceneInFocus;
}

void BattleSceneBase::OnCounter(Character& victim, Character& aggressor)
{
  AUDIO.Play(AudioType::COUNTER, AudioPriority::highest);

  if (&aggressor == player) {
    totalCounterMoves++;

    if (victim.IsDeleted()) {
      totalCounterDeletions++;
    }

    comboInfo = counterHit;
    comboInfoTimer.reset();

    field->RevealCounterFrames(true);

    // node positions are relative to the parent node's origin
    auto bounds = player->getLocalBounds();
    counterReveal.setPosition(0, -bounds.height / 4.0f);
    player->AddNode(&counterReveal);

    cardUI.SetMultiplier(2);

    // when players get hit by impact, battle scene takes back counter blessings
    player->AddDefenseRule(counterCombatRule);
  }
}

void BattleSceneBase::OnDeleteEvent(Character& pending)
{
  // Track if player is being deleted
  if (!isPlayerDeleted && player == &pending) {
    isPlayerDeleted = true;
    player = nullptr;
  }

  auto pendingPtr = &pending;

  // Find any AI using this character as a target and free that pointer  
  field->FindEntities([pendingPtr](Entity* in) {
    auto agent = dynamic_cast<Agent*>(in);

    if (agent && agent->GetTarget() == pendingPtr) {
      agent->FreeTarget();
    }

    return false;
    });

  Logger::Logf("Removing %s from battle", pending.GetName().c_str());
  mob->Forget(pending);

  if (mob->IsCleared()) {
    AUDIO.StopStream();
  }
}

const bool BattleSceneBase::IsBattleActive()
{
  return false;
}

const int BattleSceneBase::ComboDeleteSize()
{
  return comboInfoTimer.getElapsed().asSeconds() <= 1.0f ? comboDeleteCounter : 0;
}

void BattleSceneBase::HighlightTiles(bool enable)
{
  this->highlightTiles = enable;
}

void BattleSceneBase::OnCardUse(Battle::Card& card, Character& user, long long timestamp)
{
  HandleCounterLoss(user);
}

void BattleSceneBase::LoadMob(Mob& mob)
{
  this->mob = &mob;
  auto mobComps = mob.GetComponents();
  components.insert(components.end(), mobComps.begin(), mobComps.end());

  for (auto c : components) {
    c->Inject(*this);
  }

  ProcessNewestComponents();
}

void BattleSceneBase::HandleCounterLoss(Character& subject)
{
  if (&subject == player) {
    if (field->DoesRevealCounterFrames()) {
      player->RemoveNode(&counterReveal);
      player->RemoveDefenseRule(counterCombatRule);
      field->RevealCounterFrames(false);
      AUDIO.Play(AudioType::COUNTER_BONUS, AudioPriority::highest);
    }
    cardUI.SetMultiplier(1);
  }
}

void BattleSceneBase::FilterSupportCards(Battle::Card** cards, int cardCount) {
  // Only remove the support cards in the queue. Increase the previous card damage by their support value.
  int newCardCount = cardCount;
  Battle::Card* card = nullptr;

  // Create a temp card list
  Battle::Card** newCardList = new Battle::Card*[cardCount];

  int j = 0;
  for (int i = 0; i < cardCount; ) {
    if (cards[i]->IsSupport()) {
      Logger::Logf("Support card %s detected", cards[i]->GetShortName().c_str());

      if (card) {
        // support cards do not modify other support cards
        if (!card->IsSupport()) {
          int lastDamage = card->GetDamage();
          int buff = 0;

          if (cards[i]->GetShortName().substr(0, 3) == "Atk") {
            std::string substr = cards[i]->GetShortName().substr(4, cards[i]->GetShortName().size() - 4).c_str();
            buff = atoi(substr.c_str());
          }

          card->ModDamage(buff);
        }
      }

      i++;
      continue;
    }

    newCardList[j] = cards[i];
    card = cards[i];

    i++;
    j++;
  }

  newCardCount = j;

  // Set the new cards
  for (int i = 0; i < newCardCount; i++) {
    cards[i] = *(newCardList + i);
  }

  // Delete the temp list space
  // NOTE: We are _not_ deleting the pointers in them
  delete[] newCardList;

  cards = cards;
  cardCount = newCardCount;
}

#ifdef __ANDROID__
void BattleSceneBase::SetupTouchControls() {

}

void BattleSceneBase::ShutdownTouchControls() {

}
#endif

void BattleSceneBase::StartStateGraph(StateNode& start) {
  this->current = &start.state;
  this->current->onStart();
}

void BattleSceneBase::onStart()
{
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
}

void BattleSceneBase::onUpdate(double elapsed) {
  this->elapsed = elapsed;

  camera.Update((float)elapsed);
  background->Update((float)elapsed);

  if (!isPaused) {
    counterRevealAnim.Update((float)elapsed, counterReveal.getSprite());
    comboInfoTimer.update(elapsed);
    multiDeleteTimer.update(elapsed);
    battleTimer.update(elapsed);

    switch (backdropMode) {
    case backdrop::fadein:
      backdropOpacity = std::fmin(1.0, backdropOpacity + (backdropFadeSpeed * elapsed));
      break;
    case backdrop::fadeout:
      backdropOpacity = std::fmax(0.0, backdropOpacity - (backdropFadeSpeed * elapsed));
      break;
    }
  }

  // Register and eject any applicable components
  ProcessNewestComponents();

  // Update injected components
  for (auto c : components) {
    if (c->Lifetime() == Component::lifetimes::ui) {
      c->Update((float)elapsed);
    }
    else if (c->Lifetime() == Component::lifetimes::battlestep && !mob->IsCleared() && !battleTimer.isPaused()) {
      c->Update((float)elapsed);
    }
  }

  cardUI.OnUpdate((float)elapsed);
  cardCustGUI.Update((float)elapsed);

  // Track combo deletes
  if (lastMobSize != newMobSize && !isPlayerDeleted) {
    if (multiDeleteTimer.getElapsed() <= sf::seconds(COMBO_HIT_THRESHOLD_SECONDS)) {
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
    else if (multiDeleteTimer.getElapsed() > sf::seconds(COMBO_HIT_THRESHOLD_SECONDS)) {
      comboDeleteCounter = 0;
    }
  }

  if (lastMobSize != newMobSize) {
    // prepare for another enemy deletion
    multiDeleteTimer.reset();
  }

  lastMobSize = newMobSize;

  // State update
  if(!current) return;

  current->onUpdate(elapsed);

  for (auto iter = nodeToEdges.begin(); iter != nodeToEdges.end(); iter++) {
    if (iter->first == current) {
      if (iter->second->when()) {
        current->onEnd();
        current = iter->second->b;
        current->onStart();
        break;
      }
    }
  }

  // cleanup pending components
  for (auto c : deleteComponentsList) {
    delete c;
  }

  deleteComponentsList.clear();
}

void BattleSceneBase::onDraw(sf::RenderTexture& surface) {
  ENGINE.SetRenderSurface(surface);
  ENGINE.Clear();
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

    if (highlightTiles) {
      ENGINE.Draw(tile);
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
      std::sort(entitiesOnRow.begin(), entitiesOnRow.end(), [](Entity* a, Entity* b) -> bool { return a->GetLayer() > b->GetLayer(); });

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

      auto uic = entity->GetComponentsDerivedFrom<UIComponent>();

      //Logger::Log("uic size is: " + std::to_string(uic.size()));

      if (!uic.empty()) {
        ui.insert(ui.begin(), uic.begin(), uic.end());
      }

      entitiesOnRow.push_back(*entitiesIter);
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
  std::sort(entitiesOnRow.begin(), entitiesOnRow.end(), [](Entity* a, Entity* b) -> bool { return a->GetLayer() > b->GetLayer(); });

  // Now that the tiles are drawn, another pass draws the entities in sort-order
  if (mob->IsSpawningDone()) {
    // draw this row
    for (auto entity : entitiesOnRow) {
      entity->move(ENGINE.GetViewOffset());

      ENGINE.Draw(entity);

      entity->move(-ENGINE.GetViewOffset());
    }

    // Draw scene nodes
    for (auto node : scenenodes) {
      surface.draw(*node);
    }

    // Draw ui
    for (auto node : ui) {
      surface.draw(*node);
    }
  }

  // prepare for bext row
  entitiesOnRow.clear();

  if (current) current->onDraw(surface);
}

bool BattleSceneBase::IsPlayerDeleted() const
{
  return isPlayerDeleted;
}

Player* BattleSceneBase::GetPlayer()
{
  return player;
}

Field* BattleSceneBase::GetField()
{
  return field;
}

const Field* BattleSceneBase::GetField() const
{
  return field;
}

CardSelectionCust& BattleSceneBase::GetCardSelectWidget()
{
  return cardCustGUI;
}

SelectedCardsUI& BattleSceneBase::GetSelectedCardsUI() {
  return cardUI;
}

void BattleSceneBase::StartBattleTimer()
{
  battleTimer.start();
}

void BattleSceneBase::StopBattleTimer()
{
  battleTimer.pause();
}

void BattleSceneBase::BroadcastBattleStart()
{
  field->RequestBattleStart();
}

void BattleSceneBase::BroadcastBattleStop()
{
  field->RequestBattleStop();
}

const sf::Time BattleSceneBase::GetElapsedBattleTime() {
  return battleTimer.getElapsed();
}

const bool BattleSceneBase::FadeInBackdrop(double speed)
{
  backdropMode = backdrop::fadein;
  backdropFadeSpeed = speed;

  return (backdropOpacity == 0.0);
}

const bool BattleSceneBase::FadeOutBackdrop(double speed)
{
  backdropMode = backdrop::fadeout;
  backdropFadeSpeed = speed;

  return (backdropOpacity == 1.0);
}

std::vector<std::reference_wrapper<const Character>> BattleSceneBase::MobList()
{
  std::vector<std::reference_wrapper<const Character>> mobList;

  for (int i = 0; i < mob->GetMobCount(); i++) {
    mobList.push_back(mob->GetMobAt(i));
  }

  return mobList;
}

void BattleSceneBase::Quit(const FadeOut& mode) {
  if(quitting) return; 

  // end the current state
  if(current) {
    current->onEnd();
    delete current;
    current = nullptr;
  }

  // Delete all state transitions
  nodeToEdges.clear();

  for(auto&& state : states) {
    delete state;
  }

  states.clear();

  // Depending on the mode, use Swoosh's 
  // activity controller to fadeout with the right
  // visual appearance
  if(mode == FadeOut::white) {
    getController().queuePop<segue<WhiteWashFade>>();
  } else {
    getController().queuePop<segue<BlackWashFade>>();
  }

  quitting = true;
}


// What to do if we inject a card publisher, subscribe it to the main listener
void BattleSceneBase::Inject(CardUsePublisher& pub)
{
  enemyCardListener.Subscribe(pub);
  SceneNode* node = dynamic_cast<SceneNode*>(&pub);
  scenenodes.push_back(node);
}

// what to do if we inject a UIComponent, add it to the update and topmost scenenode stack
void BattleSceneBase::Inject(MobHealthUI& other)
{
  if (other.GetOwner()) other.GetOwner()->FreeComponentByID(other.GetID()); // We are owned by the scene now
  SceneNode* node = dynamic_cast<SceneNode*>(&other);
  scenenodes.push_back(node);
  components.push_back(&other);
}


// Default case: no special injection found for the type, just add it to our update loop
void BattleSceneBase::Inject(Component* other)
{
  if (other->GetOwner()) other->GetOwner()->FreeComponentByID(other->GetID());
  other->scene = this;
  components.push_back(other);
}

void BattleSceneBase::Eject(Component::ID_t ID)
{
  auto iter = std::find_if(components.begin(), components.end(), [ID](auto in) { return in->GetID() == ID; });

  if (iter != components.end()) {
    deleteComponentsList.push_back(*iter);
    components.erase(iter);
  }
}

const bool BattleSceneBase::IsCleared()
{
  return mob->IsCleared();
}

void BattleSceneBase::Link(StateNode& a, StateNode& b, ChangeCondition when) {

  nodeToEdges.insert(std::make_pair(&(a.state), new Edge(&(a.state), &(b.state), when)));
}

void BattleSceneBase::ProcessNewestComponents()
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
          if (!c) continue;

          // Older components are last in order, we're done
          if (c->GetID() <= e->lastComponentID) break;

          // Local components are not a part of the battle scene and do not get injected
          if (c->Lifetime() != Component::lifetimes::local) {
            c->Inject(*this);
          }
        }

        e->lastComponentID = latestID;
      }
    }
  }

#ifdef __ANDROID__
  SetupTouchControls();
#endif
}

void BattleSceneBase::onLeave() {
#ifdef __ANDROID__
  ShutdownTouchControls();
#endif
}

#ifdef __ANDROID__
void BattleSceneBase::SetupTouchControls() {
  /* Android touch areas*/
  TouchArea& rightSide = TouchArea::create(sf::IntRect(240, 0, 240, 320));

  rightSide.enableExtendedRelease(true);
  releasedB = false;

  rightSide.onTouch([]() {
    INPUTx.VirtualKeyEvent(InputEvent::RELEASED_A);
    });

  rightSide.onRelease([this](sf::Vector2i delta) {
    if (!releasedB) {
      INPUTx.VirtualKeyEvent(InputEvent::PRESSED_A);
    }

    releasedB = false;

    });

  rightSide.onDrag([this](sf::Vector2i delta) {
    if (delta.x < -25 && !releasedB) {
      INPUTx.VirtualKeyEvent(InputEvent::PRESSED_B);
      INPUTx.VirtualKeyEvent(InputEvent::RELEASED_B);
      releasedB = true;
    }
    });

  rightSide.onDefault([this]() {
    releasedB = false;
    });

  TouchArea& custSelectButton = TouchArea::create(sf::IntRect(100, 0, 380, 100));
  custSelectButton.onTouch([]() {
    INPUTx.VirtualKeyEvent(InputEvent::PRESSED_START);
    });
  custSelectButton.onRelease([](sf::Vector2i delta) {
    INPUTx.VirtualKeyEvent(InputEvent::RELEASED_START);
    });

  TouchArea& dpad = TouchArea::create(sf::IntRect(0, 0, 240, 320));
  dpad.enableExtendedRelease(true);
  dpad.onDrag([](sf::Vector2i delta) {
    Logger::Log("dpad delta: " + std::to_string(delta.x) + ", " + std::to_string(delta.y));

    if (delta.x > 30) {
      INPUTx.VirtualKeyEvent(InputEvent::PRESSED_RIGHT);
    }

    if (delta.x < -30) {
      INPUTx.VirtualKeyEvent(InputEvent::PRESSED_LEFT);
    }

    if (delta.y > 30) {
      INPUTx.VirtualKeyEvent(InputEvent::PRESSED_DOWN);
    }

    if (delta.y < -30) {
      INPUTx.VirtualKeyEvent(InputEvent::PRESSED_UP);
    }
    });

  dpad.onRelease([](sf::Vector2i delta) {
    if (delta.x < -30) {
      INPUTx.VirtualKeyEvent(InputEvent::RELEASED_LEFT);
    }
    });
}

void BattleSceneBase::ShutdownTouchControls() {
  TouchArea::free();
}

#endif