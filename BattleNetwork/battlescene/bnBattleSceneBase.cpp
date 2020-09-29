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


BattleSceneBase::BattleSceneBase(const BattleSceneBaseProps& props) : swoosh::Activity(&props.controller),
  player(&props.player),
  programAdvance(props.programAdvance),
  didDoubleDelete(false),
  didTripleDelete(false),
  comboDeleteCounter(0),
  totalCounterMoves(0),
  totalCounterDeletions(0),
  pauseShader(*SHADERS.GetShader(ShaderType::BLACK_FADE)),
  whiteShader(*SHADERS.GetShader(ShaderType::WHITE_FADE)),
  yellowShader(*SHADERS.GetShader(ShaderType::YELLOW)),
  customBarShader(*SHADERS.GetShader(ShaderType::CUSTOM_BAR)),
  heatShader(*SHADERS.GetShader(ShaderType::SPOT_DISTORTION)),
  iceShader(*SHADERS.GetShader(ShaderType::SPOT_REFLECTION)),
  distortionMap(*TEXTURES.GetTexture(TextureType::HEAT_TEXTURE)),
  cardListener(props.player),
  // cap of 8 cards, 8 cards drawn per turn
  cardCustGUI(props.folder.Clone(), 8, 8),
  camera(*ENGINE.GetCamera()),
  cardUI(player) {

  if (props.mob.GetMobCount() == 0) {
    Logger::Log(std::string("Warning: Mob was empty when battle started. Mob Type: ") + typeid(mob).name());
  }

  /*
  Set Scene*/
  field = props.mob.GetField();
  CharacterDeleteListener::Subscribe(*field);

  player->ChangeState<PlayerIdleState>();
  player->ToggleTimeFreeze(false);
  field->AddEntity(*player, 2, 2);

  // Card UI for player
  cardListener.Subscribe(cardUI);
  this->CardUseListener::Subscribe(cardUI);

  /*
  Background for scene*/
  background = mob->GetBackground();

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

  components = mob->GetComponents();

  PlayerHealthUI* healthUI = new PlayerHealthUI(player);
  cardCustGUI.AddNode(healthUI);
  components.push_back((UIComponent*)healthUI);

  for (auto c : components) {
    c->Inject(*this);
  }

  ProcessNewestComponents();

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

  // CHIP CUST GRAPHICS
  customBarTexture = TEXTURES.LoadTextureFromFile("resources/ui/custom.png");
  customBarSprite.setTexture(customBarTexture);
  customBarSprite.setOrigin(customBarSprite.getLocalBounds().width / 2, 0);
  customBarPos = sf::Vector2f(240.f, 0.f);
  customBarSprite.setPosition(customBarPos);
  customBarSprite.setScale(2.f, 2.f);

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

  isSceneInFocus = false;
}

BattleSceneBase::~BattleSceneBase() {};

const int BattleSceneBase::GetCounterCount() const {
  return totalCounterMoves;
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

void BattleSceneBase::OnCardUse(Battle::Card& card, Character& user, long long timestamp)
{
  HandleCounterLoss(user);
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
  Battle::Card** newCardList = new Battle::Card * [cardCount];

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

void BattleSceneBase::onUpdate(double elapsed) {
  this->elapsed = elapsed;


  camera.Update((float)elapsed);
  background->Update((float)elapsed);

  if (!isPaused) {
    counterRevealAnim.Update((float)elapsed, counterReveal.getSprite());
    comboInfoTimer.update(elapsed);
    multiDeleteTimer.update(elapsed);
    battleTimer.update(elapsed);
  }

  // Register and eject any applicable components
  ProcessNewestComponents();

  // Update components
  for (auto c : components) {
    c->OnUpdate((float)elapsed);
  }

  cardUI.OnUpdate((float)elapsed);

  // State update

  if(!current) return;

  current->onUpdate(elapsed);

  auto options = nodeToEdges.equal_range(current);

  for(auto iter = options.first; iter != options.second; iter++) {
    if(iter->second.when()) {
      current->onEnd();
      current = &iter->second.b.get().state;
      current->onStart();
    }
  }
}

void BattleSceneBase::onDraw(sf::RenderTexture& surface) {
  if (current) current->onDraw(surface);
}

Field* BattleSceneBase::GetField()
{
  return field;
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
    getController().queuePop<WhiteWashFade>();
  } else {
    getController().queuePop<BlackWashFade>();
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
  if (!other) return;

  if (other->GetOwner()) other->GetOwner()->FreeComponentByID(other->GetID());
  components.push_back(other);
}

void BattleSceneBase::Eject(Component::ID_t ID)
{
  auto iter = std::find_if(components.begin(), components.end(), [ID](auto in) { return in->GetID() == ID; });

  if (iter != components.end()) {
    components.erase(iter);
  }
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

const bool BattleSceneBase::IsCleared()
{
  return mob->IsCleared();
}

void BattleSceneBase::Link(StateNode& a, StateNode& b, ChangeCondition&& when) {
  Edge edge{a, b, std::move(when)};
  nodeToEdges.insert(std::make_pair(&a.state, edge));
  nodeToEdges.insert(std::make_pair(&b.state, edge));
}