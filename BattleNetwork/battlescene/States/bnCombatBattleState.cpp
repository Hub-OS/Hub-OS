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

CombatBattleState::CombatBattleState(Mob* mob, std::vector<std::shared_ptr<Player>>& tracked, double customDuration) :
  mob(mob), 
  tracked(tracked), 
  customDuration(customDuration),
  customBarShader(Shaders().GetShader(ShaderType::CUSTOM_BAR)),
  pauseShader(Shaders().GetShader(ShaderType::BLACK_FADE))
{
  // PAUSE
  pause.setTexture(*Textures().LoadFromFile("resources/ui/pause.png"));
  pause.setScale(2.f, 2.f);
  pause.setOrigin(pause.getLocalBounds().width * 0.5f, pause.getLocalBounds().height * 0.5f);
  pause.setPosition(sf::Vector2f(240.f, 145.f));

  // CHIP CUST GRAPHICS
  auto customBarTexture = Textures().LoadFromFile("resources/ui/custom.png");
  customBar.setTexture(customBarTexture);
  customBar.setOrigin(customBar.getLocalBounds().width / 2, 0);
  auto customBarPos = sf::Vector2f(240.f, 0.f);
  customBar.setPosition(customBarPos);
  customBar.setScale(2.f, 2.f);

  if (pauseShader) {
    pauseShader->setUniform("texture", sf::Shader::CurrentTexture);
    pauseShader->setUniform("opacity", 0.25f);
  }

  if (customBarShader) {
    customBarShader->setUniform("texture", sf::Shader::CurrentTexture);
    customBarShader->setUniform("factor", 0);
    customBar.SetShader(customBarShader);
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
}

const bool CombatBattleState::HasTimeFreeze() const {
  return hasTimeFreeze && !isPaused;
}

const bool CombatBattleState::PlayerWon() const
{
  auto blueTeamCharCount = 0;
  GetScene().GetField()->FindEntities([&blueTeamCharCount](std::shared_ptr<Entity>& e) {
    // check when the enemy has been removed from the field even if the mob
    // forgot about it
    // TODO: do not use dynamic casts
    if (e->GetTeam() == Team::blue && dynamic_cast<Character*>(e.get()) && (dynamic_cast<Obstacle*>(e.get()) == nullptr)) {
      blueTeamCharCount++;
    }

    return false;
  });

  return !PlayerLost() && mob->IsCleared() && blueTeamCharCount == 0 && GetScene().ComboDeleteSize() == 0;
}

const bool CombatBattleState::PlayerLost() const
{
  return GetScene().IsPlayerDeleted();
}

const bool CombatBattleState::PlayerRequestCardSelect()
{
  return !this->isPaused && this->isGaugeFull && !mob->IsCleared() && Input().Has(InputEvents::pressed_cust_menu);
}

void CombatBattleState::EnablePausing(bool enable)
{
  canPause = enable;
}

void CombatBattleState::SkipFrame()
{
  skipFrame = true;
}

void CombatBattleState::onStart(const BattleSceneState* last)
{
  GetScene().SetCustomBarProgress(customDuration);
  GetScene().HighlightTiles(true); // re-enable tile highlighting

  // tracked[0] should be the client player
  if ((tracked[0]->GetHealth() > 0) && this->HandleNextRoundSetup(last)) {
    GetScene().StartBattleStepTimer();
    GetScene().GetField()->ToggleTimeFreeze(false);

    hasTimeFreeze = false;

    // reset bar and related flags
    customProgress = 0;

    if (customBarShader) {
      customBarShader->setUniform("factor", 0);
    }

    isGaugeFull = false;
  }
}

void CombatBattleState::onEnd(const BattleSceneState* next)
{
  // If the next state is not a substate of combat
  // then reset our combat and custom progress values
  if (this->HandleNextRoundSetup(next)) {
    GetScene().StopBattleStepTimer();

    // reset bar 
    customProgress = 0;

    if (customBarShader) {
      customBarShader->setUniform("factor", 0);
    }
  }

  GetScene().HighlightTiles(false);
  hasTimeFreeze = false; 
}

void CombatBattleState::onUpdate(double elapsed)
{  
  if (skipFrame) {
    skipFrame = false;
    return;
  }

  GetScene().IncrementFrame();

  if ((mob->IsCleared() || tracked[0]->GetHealth() == 0 )&& !clearedMob) {
    auto cardUI = tracked[0]->GetFirstComponent<PlayerSelectedCardsUI>();

    if (cardUI) {
      cardUI->Hide();
    }

    GetScene().BroadcastBattleStop();

    clearedMob = true;
  }

  if (canPause && Input().Has(InputEvents::pressed_pause) && !mob->IsCleared()) {
    if (isPaused) {
      // unpauses
      // Require to stop the battle step timer and all battle-related component updates
      GetScene().StartBattleStepTimer();
    }
    else {
      // pauses
      Audio().Play(AudioType::PAUSE);

      // Require to start the timer again and all battle-related component updates
      GetScene().StopBattleStepTimer();
    }

    // toggles 
    isPaused = !isPaused;
  }

  if (isPaused) return; // do not update anything else

  customProgress += elapsed;
  GetScene().SetCustomBarProgress(customProgress/customDuration);

  // Update the field. This includes the player.
  // After this function, the player may have used a card.
  GetScene().GetField()->Update((float)elapsed);

  if (customProgress / customDuration >= 1.0 && !isGaugeFull) {
    isGaugeFull = true;
    Audio().Play(AudioType::CUSTOM_BAR_FULL);
  }

  if (customBarShader) {
    customBarShader->setUniform("factor", (float)(customProgress / customDuration));
  }
}

void CombatBattleState::onDraw(sf::RenderTexture& surface)
{
  const int comboDeleteSize = GetScene().ComboDeleteSize();

  if (!GetScene().Countered()) {
    if (comboDeleteSize == 2) {
      surface.draw(doubleDelete);
    } else if(comboDeleteSize > 2) {
      surface.draw(tripleDelete);
    }
  }
  else {
    surface.draw(counterHit);
  }

  surface.draw(GetScene().GetCardSelectWidget());

  if (!mob->IsCleared()) {
    surface.draw(customBar);
  }

  if (isPaused) {
    // render on top
    surface.draw(pause);
  }
}

void CombatBattleState::OnCardActionUsed(std::shared_ptr<CardAction> action, uint64_t timestamp)
{
  if (!mob->IsCleared()) {
    hasTimeFreeze = action->GetMetaData().timeFreeze;
  }
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
