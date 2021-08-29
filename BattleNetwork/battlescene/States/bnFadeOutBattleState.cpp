#include "bnFadeOutBattleState.h"
#include "../bnBattleSceneBase.h"

#include "../../bnPlayer.h"
#include "../../bnField.h"

FadeOutBattleState::FadeOutBattleState(const FadeOut& mode, std::vector<Player*>& tracked) : mode(mode), tracked(tracked) {}

void FadeOutBattleState::onStart(const BattleSceneState*) {
  for (auto p : tracked) {
    p->ChangeState<PlayerIdleState>();
  }

  auto* field = GetScene().GetField();
  field->RequestBattleStop();

  auto& mobList = GetScene().MobList();

  if (mobList.empty())
    return;

  auto& results = GetScene().BattleResultsObj();
  results.turns = GetScene().GetTurnCount();

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
  wait -= elapsed;

  if (wait <= 0.0) {
    GetScene().Quit(mode);
  }
  else if(keepPlaying) {
    GetScene().GetField()->Update(elapsed);
  }
}

void FadeOutBattleState::onDraw(sf::RenderTexture&)
{
}

void FadeOutBattleState::EnableKeepPlaying(bool enable)
{
  this->keepPlaying = enable;
}
