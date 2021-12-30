#include "bnFadeOutBattleState.h"
#include "../bnBattleSceneBase.h"

#include "../../bnPlayer.h"
#include "../../bnField.h"

FadeOutBattleState::FadeOutBattleState(const FadeOut& mode) : mode(mode) {}

void FadeOutBattleState::onStart(const BattleSceneState*) {
  BattleSceneBase& scene = GetScene();
  std::shared_ptr<Player> localPlayer = scene.GetLocalPlayer();
  Team localTeam = localPlayer->GetTeam();

  for (auto p : scene.GetAllPlayers()) {
    p->ChangeState<PlayerIdleState>();
  }

  auto field = scene.GetField();
  field->RequestBattleStop();

  auto mobList = localTeam == Team::red ? scene.RedTeamMobList() : scene.BlueTeamMobList();

  if (mobList.empty())
    return;

  BattleResults& results = scene.BattleResultsObj();
  results.turns = scene.GetTurnCount();

  Entity::ID_t next_id{mobList.front().get().GetID()};

  for (auto& enemy : mobList) {
    BattleResults::MobData mob;
    mob.health = enemy.get().GetHealth();
    mob.id = enemy.get().GetName();

    // track deleted enemies from this mob by returning empty values where they would be
    while (enemy.get().GetID() != next_id) {
      results.mobStatus.push_back(BattleResults::MobData{});
      next_id++;
    }

    results.mobStatus.push_back(mob);
    next_id++;
  }
}

void FadeOutBattleState::onEnd(const BattleSceneState*)
{
}

void FadeOutBattleState::onUpdate(double elapsed)
{
  BattleSceneBase& scene = GetScene();
  wait -= elapsed;

  if (wait <= 0.0) {
    scene.Quit(mode);
  }
  else if(keepPlaying) {
    scene.GetField()->Update(elapsed);
  }
}

void FadeOutBattleState::onDraw(sf::RenderTexture&)
{
}

void FadeOutBattleState::EnableKeepPlaying(bool enable)
{
  this->keepPlaying = enable;
}
