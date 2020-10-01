#include "bnRewardBattleState.h"

#include "../../bnBattleResults.h"
#include "../../bnPlayer.h"
#include "../../bnMob.h"
#include "../battlescene/bnBattleSceneBaseBase.h"

RewardBattleState::RewardBattleState(Player* player, Mob* mob) : player(player), mob(mob)
{ 
}

void RewardBattleState::onStart()
{
  auto battleTime = GetScene().GetElapsedBattleTime();
  int moveCount = player->GetMoveCount();
  int hitCount = player->GetHitCount();
  int counterCount = GetScene().GetCounterCount();
  bool doubleDelete = GetScene().DoubleDelete();
  bool tripleDelete = GetScene().TripleDelete();


  battleResults = new BattleResults(GetScene().GetElapsedBattleTime(), moveCount, hitCount, counterCount, doubleDelete, tripleDelete, mob);
}

void RewardBattleState::onUpdate(double elapsed)
{
  if (battleResults) battleResults->Update(elapsed);
}

void RewardBattleState::onDraw(sf::RenderTexture& surface)
{
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

bool RewardBattleState::OKIsPressed() {
  return true;
}