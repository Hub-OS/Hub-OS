#include "bnCombatBattleState.h"

#include "../bnBattleSceneBase.h"

#include "../../bnMob.h"
#include "../../bnTeam.h"
#include "../../bnEntity.h"
#include "../../bnCharacter.h"
#include "../../bnObstacle.h"
#include "../../bnCard.h"
#include "../../bnCardAction.h"
#include "../../bnInputManager.h"
#include "../../bnShaderResourceManager.h"

CombatBattleState::CombatBattleState(Mob* mob, std::vector<Player*> tracked, double customDuration) 
  : mob(mob), 
    tracked(tracked), 
    customDuration(customDuration),
    customBarShader(*SHADERS.GetShader(ShaderType::CUSTOM_BAR)),
    pauseShader(*SHADERS.GetShader(ShaderType::BLACK_FADE))
{
  // PAUSE
  pauseFont = TEXTURES.LoadFontFromFile("resources/fonts/dr_cain_terminal.ttf");
  pauseLabel = sf::Text("paused", *pauseFont);
  pauseLabel.setOrigin(pauseLabel.getLocalBounds().width / 2, pauseLabel.getLocalBounds().height * 2);
  pauseLabel.setPosition(sf::Vector2f(240.f, 160.f));

  // CHIP CUST GRAPHICS
  auto customBarTexture = TEXTURES.LoadTextureFromFile("resources/ui/custom.png");
  customBar.setTexture(customBarTexture);
  customBar.setOrigin(customBar.getLocalBounds().width / 2, 0);
  auto customBarPos = sf::Vector2f(240.f, 0.f);
  customBar.setPosition(customBarPos);
  customBar.setScale(2.f, 2.f);

  pauseShader.setUniform("texture", sf::Shader::CurrentTexture);
  pauseShader.setUniform("opacity", 0.25f);

  customBarShader.setUniform("texture", sf::Shader::CurrentTexture);
  customBarShader.setUniform("factor", 0);
  customBar.SetShader(&customBarShader);

  // COMBO DELETE AND COUNTER LABELS
  auto labelPosition = sf::Vector2f(240.0f, 50.f);

  doubleDelete = sf::Sprite(*LOAD_TEXTURE(DOUBLE_DELETE));
  doubleDelete.setOrigin(doubleDelete.getLocalBounds().width / 2.0f, doubleDelete.getLocalBounds().height / 2.0f);
  doubleDelete.setPosition(labelPosition);
  doubleDelete.setScale(2.f, 2.f);

  tripleDelete = doubleDelete;
  tripleDelete.setTexture(*LOAD_TEXTURE(TRIPLE_DELETE));

  counterHit = doubleDelete;
  counterHit.setTexture(*LOAD_TEXTURE(COUNTER_HIT));
}

const bool CombatBattleState::HasTimeFreeze() const {
  return hasTimeFreeze && !isPaused;
}

const bool CombatBattleState::PlayerWon() const
{
  auto blueTeamChars = GetScene().GetField()->FindEntities([](Entity* e) {
    // check when the enemy has been removed from the field even if the mob
    // forgot about it
    // TODO: do not use dynamic casts
    return e->GetTeam() == Team::blue && dynamic_cast<Character*>(e) && (dynamic_cast<Obstacle*>(e) == nullptr);
  });

  return !PlayerLost() && mob->IsCleared() && blueTeamChars.empty();
}

const bool CombatBattleState::PlayerLost() const
{
  return GetScene().IsPlayerDeleted();
}

const bool CombatBattleState::PlayerRequestCardSelect()
{
  return !this->isPaused && this->isGaugeFull && !mob->IsCleared() && INPUTx.Has(EventTypes::PRESSED_CUST_MENU);
}

void CombatBattleState::onStart(const BattleSceneState* last)
{
  GetScene().HighlightTiles(true); // re-enable tile highlighting

  // tracked[0] should be the client player
  if ((tracked[0]->GetHealth() > 0) && this->HandleNextRoundSetup(last)) {
    GetScene().StartBattleStepTimer();
    GetScene().GetField()->ToggleTimeFreeze(false);
    tracked[0]->ChangeState<PlayerControlledState>();

    hasTimeFreeze = false;

    // reset bar and related flags
    customProgress = 0;
    customBarShader.setUniform("factor", 0);
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
    customBarShader.setUniform("factor", 0);
  }

  GetScene().HighlightTiles(false);
  hasTimeFreeze = false; 
}

void CombatBattleState::onUpdate(double elapsed)
{  
  if ((mob->IsCleared() || tracked[0]->GetHealth() == 0 )&& !clearedMob) {
    auto cardUI = tracked[0]->GetFirstComponent<SelectedCardsUI>();

    if (cardUI) {
      tracked[0]->FreeComponentByID(cardUI->GetID());
    }

    GetScene().BroadcastBattleStop();

    clearedMob = true;
  }

  if (INPUTx.Has(EventTypes::PRESSED_PAUSE) && !mob->IsCleared()) {

    if (isPaused) {
      // unpauses

      ENGINE.RevokeShader();

      // Require to stop the battle step timer and all battle-related component updates
      GetScene().StartBattleStepTimer();
    }
    else {
      // pauses

      AUDIO.Play(AudioType::PAUSE);

      // Require to start the timer again and all battle-related component updates
      GetScene().StopBattleStepTimer();
    }

    // toggles 
    isPaused = !isPaused;
  }

  if (isPaused) return; // do not update anything else

  customProgress += elapsed;

  // Update the field. This includes the player.
  // After this function, the player may have used a card.
  GetScene().GetField()->Update((float)elapsed);

  /**
    Cards are executed at the end of the battle frame (this combat state update begins the end)
    After the player controller dequeue's and registers a card action, we must be the authority to execute it
  */
  if (!HasTimeFreeze()) {
    auto actions = tracked[0]->GetComponentsDerivedFrom<CardAction>();
    if (actions.size() > 0 && actions[0]->CanExecute()) {
      actions[0]->OnExecute();
    }
  }

  if (customProgress / customDuration >= 1.0 && !isGaugeFull) {
    isGaugeFull = true;
    AUDIO.Play(AudioType::CUSTOM_BAR_FULL);
  }

  customBarShader.setUniform("factor", (float)(customProgress / customDuration));
}

void CombatBattleState::onDraw(sf::RenderTexture& surface)
{
  const int comboDeleteSize = GetScene().ComboDeleteSize();

  if (!GetScene().Countered()) {
    if (comboDeleteSize == 2) {
      ENGINE.Draw(doubleDelete);
    } else if(comboDeleteSize > 2) {
      ENGINE.Draw(tripleDelete);
    }
  }
  else {
    ENGINE.Draw(counterHit);
  }

  ENGINE.Draw(GetScene().GetCardSelectWidget());

  if (!mob->IsCleared()) {
    ENGINE.Draw(&customBar);
  }

  if (isPaused) {
    ENGINE.SetShader(&pauseShader);

    // render on top
    ENGINE.Draw(pauseLabel, false);
  }
}

void CombatBattleState::OnCardUse(Battle::Card& card, Character& user, long long timestamp)
{
  if (!mob->IsCleared()) {
    hasTimeFreeze = card.IsTimeFreeze();
  }
}

const bool CombatBattleState::HandleNextRoundSetup(const BattleSceneState* state)
{
  auto iter = std::find_if(subcombatStates.begin(), subcombatStates.end(),
    [state](const BattleSceneState* in) {
      return in == state;
    }
  );

  // Only stop battlestep timers and effects for states
  // that are not part of the combat state list
  return (iter == subcombatStates.end());
}
