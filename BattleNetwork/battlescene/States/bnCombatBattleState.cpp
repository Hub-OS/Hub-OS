#include "bnCombatBattleState.h"

#include "../bnBattleSceneBase.h"

#include "../../bnPlayer.h"
#include "../../bnPlayerSelectedCardsUI.h"
#include "../../bnMob.h"
#include "../../bnTeam.h"
#include "../../bnEntity.h"
#include "../../bnCharacter.h"
#include "../../bnObstacle.h"
#include "../../bnCard.h"
#include "../../bnCardAction.h"
#include "../../bnInputManager.h"
#include "../../bnShaderResourceManager.h"

CombatBattleState::CombatBattleState(frame_time_t customDuration) :
  pauseShader(Shaders().GetShader(ShaderType::BLACK_FADE)),
  summonsLabel(std::make_shared<Text>(Font::Style::thick)),
  summonsLabelShadow(std::make_shared<Text>(Font::Style::thick)),
  dmg(std::make_shared<Text>(Font::Style::gradient_orange)),
  multiplier(std::make_shared<Text>(Font::Style::thick)),
  multiplierShadow(std::make_shared<Text>(Font::Style::thick))
{
  // PAUSE
  pause.setTexture(*Textures().LoadFromFile("resources/ui/pause.png"));
  pause.setScale(2.f, 2.f);
  pause.setOrigin(pause.getLocalBounds().width * 0.5f, pause.getLocalBounds().height * 0.5f);
  pause.setPosition(sf::Vector2f(240.f, 145.f));
 
  if (pauseShader) {
    pauseShader->setUniform("texture", sf::Shader::CurrentTexture);
    pauseShader->setUniform("opacity", 0.25f);
  }

  // COMBO DELETE AND COUNTER LABELS
  auto labelPosition = sf::Vector2f(240.0f, 50.f);

  doubleDelete = sf::Sprite(*Textures().LoadFromFile(TexturePaths::DOUBLE_DELETE));
  doubleDelete.setOrigin(doubleDelete.getLocalBounds().width / 2.0f, doubleDelete.getLocalBounds().height / 2.0f);
  doubleDelete.setPosition(labelPosition);
  doubleDelete.setScale(2.f, 2.f);

  tripleDelete = doubleDelete;
  tripleDelete.setTexture(*Textures().LoadFromFile(TexturePaths::TRIPLE_DELETE));

  counterHit = doubleDelete;
  counterHit.setTexture(*Textures().LoadFromFile(TexturePaths::COUNTER_HIT));

  //PVP Labels and such
  summonsLabelShadow->SetColor(sf::Color::Black);
  multiplierShadow->SetColor(sf::Color::Black);
  cardText.AddNode(summonsLabelShadow);
  cardText.AddNode(summonsLabel);
  cardText.AddNode(dmg);
  cardText.AddNode(multiplierShadow);
  cardText.AddNode(multiplier);
  summonsLabelShadow->setScale({ 2.0f, 2.0f });
  summonsLabel->setScale({ 2.0f, 2.0f });
  dmg->setScale({ 2.0f, 2.0f });
  multiplierShadow->setScale({ 2.0f, 2.0f });
  multiplier->setScale({ 2.0f, 2.0f });
}

const bool CombatBattleState::IsMobCleared() const
{
  const BattleSceneBase& scene = GetScene();
  bool redTeamCleared = scene.IsRedTeamCleared();
  bool blueTeamCleared = scene.IsBlueTeamCleared();
  bool isCleared = false;

  Team localTeam = scene.GetLocalPlayer()->GetTeam();

  if (localTeam == Team::red) {
    isCleared = blueTeamCleared;
  }
  else if (localTeam == Team::blue) {
    isCleared = redTeamCleared;
  }
  else {
    // unknown
    isCleared = blueTeamCleared && redTeamCleared;
  }

  return isCleared;
}

const bool CombatBattleState::HasTimeFreeze() const {
  return hasTimeFreeze && !isPaused;
}

const bool CombatBattleState::RedTeamWon() const
{
  // we add `ComboDeleteSize() == 0` to our state check to give us a few more frames of display before switching states
  return GetScene().IsBlueTeamCleared() && GetScene().ComboDeleteSize() == 0;
}

const bool CombatBattleState::BlueTeamWon() const
{
  return GetScene().IsRedTeamCleared() && GetScene().ComboDeleteSize() == 0;
}

const bool CombatBattleState::PlayerDeleted() const
{
  return GetScene().IsPlayerDeleted();
}

const bool CombatBattleState::PlayerRequestCardSelect()
{
  BattleSceneBase& scene = GetScene();
  return !this->isPaused && scene.IsCustGaugeFull() && !IsMobCleared() && scene.GetLocalPlayer()->InputState().Has(InputEvents::pressed_cust_menu);
}

void CombatBattleState::EnablePausing(bool enable)
{
  canPause = enable;
}

void CombatBattleState::onStart(const BattleSceneState* last)
{
  BattleSceneBase& scene = GetScene();
  scene.HighlightTiles(true); // re-enable tile highlighting

  if ((scene.GetLocalPlayer()->GetHealth() > 0) && this->HandleNextRoundSetup(last)) {
    scene.StartBattleStepTimer();
    scene.GetField()->ToggleTimeFreeze(false);

    hasTimeFreeze = false;

    // reset bar and related flags
    scene.SetCustomBarProgress(frames(0));
  }
}

void CombatBattleState::onEnd(const BattleSceneState* next)
{
  BattleSceneBase& scene = GetScene();

  // If the next state is not a substate of combat
  // then reset our combat and custom progress values
  if (this->HandleNextRoundSetup(next)) {
    scene.StopBattleStepTimer();

    // reset bar 
    scene.SetCustomBarProgress(frames(0));
  }

  scene.HighlightTiles(false);
  hasTimeFreeze = false; 
}

void CombatBattleState::onUpdate(double elapsed)
{  
  BattleSceneBase& scene = GetScene();
  Player& player = *GetScene().GetLocalPlayer();

  if ((IsMobCleared() || player.GetHealth() == 0 )&& !clearedMob) {
    std::shared_ptr<PlayerSelectedCardsUI> cardUI = player.GetFirstComponent<PlayerSelectedCardsUI>();

    if (cardUI) {
      cardUI->Hide();
    }

    scene.BroadcastBattleStop();

    clearedMob = true;
  }

  if (canPause && Input().Has(InputEvents::pressed_pause) && !IsMobCleared()) {
    if (isPaused) {
      // unpauses
      // Require to stop the battle step timer and all battle-related component updates
      scene.StartBattleStepTimer();
    }
    else {
      // pauses
      Audio().Play(AudioType::PAUSE);

      // Require to start the timer again and all battle-related component updates
      scene.StopBattleStepTimer();
    }

    // toggles 
    isPaused = !isPaused;
  }

  if (isPaused) return; // do not update anything else

  scene.SetCustomBarProgress(scene.GetCustomBarProgress() + frames(1));

  // Update the field. This includes the player.
  // After this function, the player may have used a card.
  scene.GetField()->Update((float)elapsed);
}

void CombatBattleState::onDraw(sf::RenderTexture& surface)
{
    BattleSceneBase& scene = GetScene();
    const int comboDeleteSize = scene.ComboDeleteSize();

    if (!scene.Countered()) {
        if (comboDeleteSize == 2) {
            surface.draw(doubleDelete);
        }
        else if (comboDeleteSize > 2) {
            surface.draw(tripleDelete);
        }
    }
    else {
        surface.draw(counterHit);
    }

    surface.draw(scene.GetCardSelectWidget());

    scene.DrawCustGauage(surface);
    auto otherPlayers = scene.GetOtherPlayers();

    //If we are in pvp (other players are existing) draw cards for them
    if (otherPlayers.size() > 0) {
        for (std::shared_ptr<Player> player : otherPlayers) {
            DrawCardData(surface, player);
        }
    }

    if (isPaused) {
        // render on top
        surface.draw(pause);
    }
}

void CombatBattleState::OnCardActionUsed(std::shared_ptr<CardAction> action, uint64_t timestamp)
{
  BattleSceneBase& scene = GetScene();
  // Only intercept this event if we are active
  if (scene.GetCurrentState() != this) return;

  Logger::Logf(LogLevel::debug, "CombatBattleState::OnCardActionUsed() on frame #%i", scene.FrameNumber().count());
  if (!IsMobCleared()) {
    hasTimeFreeze = action->GetMetaData().GetProps().timeFreeze;
  }
}

void CombatBattleState::DrawCardData(sf::RenderTarget& target, std::shared_ptr<Player> player)
{
    //Do nothing if the player is null.
    if (player == nullptr) {
        return;
    }

    //If the action is time freeze it will be drawn by time freeze state
    //If the action is nothing what are we doing don't draw it
    //If the action's animation is over, stop drawing it. NOTE: Add a hang time where we keep drawing it for a bit
    //This is so that fast actions are visible for, say, 30 frames.
    std::shared_ptr<CardAction> action = player->CurrentCardAction();
    if (action == nullptr || action->GetMetaData().IsTimeFreeze() || action->IsAnimationOver()) {
        return;
    }

    //If the action exists and is registered to not show it's name, don't continue.
    //If we are the same team as the action, don't continue.
    Team team = action->GetActor()->GetTeam();
    if (!action->IsShowName() || player->GetTeam() == action->GetActor()->GetTeam()) {
        return;
    }

    BattleSceneBase& scene = GetScene();

    bool canBoost{};
    float multiplierOffset = 0.f;
    float dmgOffset = 0.f;

    Team localTeam = scene.GetLocalPlayer()->GetTeam();
    sf::Vector2f position = sf::Vector2f(64.f, 64.f);
    if (player->GetTeam() != localTeam) {
        position = sf::Vector2f(256.f, 64.f);
    }

    summonsLabelShadow->SetString(action->GetMetaData().GetShortName());
    summonsLabel->SetString(action->GetMetaData().GetShortName());

    dmg->SetString("");

    multiplierShadow->SetString("");
    multiplier->SetString("");

    const Battle::Card& card = action->GetMetaData();
    canBoost = card.CanBoost();

    // Calculate the delta damage values to correctly draw the modifiers
    const unsigned int multiplierValue = card.GetMultiplier();
    int unmodDamage = card.GetBaseProps().damage;
    int damage = card.GetProps().damage;

    if (multiplierValue) {
        damage /= multiplierValue;
    }

    //Determine damage text
    int delta = damage - unmodDamage;
    sf::String dmgText = std::to_string(unmodDamage);
    if (delta != 0) {
        dmgText = dmgText + sf::String("+") + sf::String(std::to_string(std::abs(delta)));
    }

    // attacks that normally show no damage will show if the modifer adds damage
    if (delta > 0 || unmodDamage > 0) {
        dmg->SetString(dmgText);
        dmgOffset = 10.0f;
    }

    //Dawn: Only show multiplier if value is greater than 1. Don't need to show x0 or x1.
    if (multiplierValue > 1 && unmodDamage != 0) {
        // add "x N" where N is the multiplier
        std::string multStr = "x" + std::to_string(multiplierValue);
        multiplierShadow->SetString(multStr);
        multiplier->SetString(multStr);
        multiplierOffset = 10.0f;
    }

    //Set the position of the label
    //Label position gets overwritten by the last player to use the card, favoring the other team.
    cardText.setPosition(position);
    auto textPos = cardText.getPosition();
    summonsLabelShadow->setPosition(textPos.x + 2.f, textPos.y + 2.f);
    summonsLabel->setPosition(textPos.x, textPos.y);
    dmg->setPosition(textPos.x + summonsLabel->GetWorldBounds().width + dmgOffset, textPos.y);
    multiplierShadow->setPosition(textPos.x + summonsLabel->GetWorldBounds().width + dmg->GetWorldBounds().width + multiplierOffset + 2.f, textPos.y + 2.f);
    multiplier->setPosition(multiplierShadow->getPosition().x - 2.f, multiplierShadow->getPosition().y - 2.f);

    // shadow beneath does not get drawn; had to add secondary text effects. much duplication. is there a solution?
    scene.DrawWithPerspective(cardText, target);
}


const bool CombatBattleState::HandleNextRoundSetup(const BattleSceneState* state)
{
  // Only stop battlestep timers and effects for states
  // that are not part of the combat state list
  return !IsStateCombat(state);
}

const bool CombatBattleState::IsStateCombat(const BattleSceneState* state)
{
  auto iter = std::find_if(subcombatStates.begin(), subcombatStates.end(),
    [state](const BattleSceneState* in) {
      return in == state;
    }
  );

  return (iter != subcombatStates.end()) || (state == this);
}
