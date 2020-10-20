#include "bnCombatBattleState.h"

#include "../bnBattleSceneBase.h"

#include "../../bnMob.h"
#include "../../bnTeam.h"
#include "../../bnEntity.h"
#include "../../bnCharacter.h"
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
}

const bool CombatBattleState::HasTimeFreeze() const {
  return false;
  // TODO: mark true when a used chip has TimeFreeze
}

const bool CombatBattleState::PlayerWon() const
{
  auto blueTeamChars = GetScene().GetField()->FindEntities([](Entity* e) {
    return e->GetTeam() == Team::blue && dynamic_cast<Character*>(e);
  });

  return !PlayerLost() && mob->IsCleared() && blueTeamChars.empty();
}

const bool CombatBattleState::PlayerLost() const
{
  return GetScene().IsPlayerDeleted();
}

const bool CombatBattleState::PlayerRequestCardSelect()
{
  return !this->isPaused && this->isGaugeFull && INPUTx.Has(EventTypes::PRESSED_CUST_MENU);
}

void CombatBattleState::onStart()
{
  GetScene().StartBattleTimer();
  
  tracked[0]->ChangeState<PlayerControlledState>();

  GetScene().GetField()->ToggleTimeFreeze(false);
}

void CombatBattleState::onEnd()
{
  GetScene().StopBattleTimer();
  customProgress = 0;
  customBarShader.setUniform("factor", 0);
  isGaugeFull = false;
}

void CombatBattleState::onUpdate(double elapsed)
{
  if (INPUTx.Has(EventTypes::PRESSED_PAUSE)) {
    isPaused = !isPaused;

    if (!isPaused) {
      ENGINE.RevokeShader();
    }
    else {
      AUDIO.Play(AudioType::PAUSE);
    }
  }

  if (isPaused) return; // do not update

  customProgress += elapsed;

  GetScene().GetField()->Update((float)elapsed);

  if (customProgress / customDuration >= 1.0 && !isGaugeFull) {
    isGaugeFull = true;
    AUDIO.Play(AudioType::CUSTOM_BAR_FULL);
  }

  customBarShader.setUniform("factor", (float)(customProgress / customDuration));
}

void CombatBattleState::onDraw(sf::RenderTexture& surface)
{
  const int comboDeleteSize = GetScene().ComboDeleteSize();

  switch(comboDeleteSize){
  case 2:
    ENGINE.Draw(doubleDelete);
    break;
  default:
    ENGINE.Draw(tripleDelete);
  }

  ENGINE.Draw(GetScene().GetCardSelectWidget());

  ENGINE.Draw(&customBar);

  if (isPaused) {
    ENGINE.SetShader(&pauseShader);

    // render on top
    ENGINE.Draw(pauseLabel, false);
  }
}