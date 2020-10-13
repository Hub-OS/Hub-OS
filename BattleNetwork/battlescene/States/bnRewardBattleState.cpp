#include "bnRewardBattleState.h"

#include "../../bnBattleResults.h"
#include "../../bnPlayer.h"
#include "../../bnMob.h"
#include "../../bnInputManager.h"
#include "../bnBattleSceneBase.h"

#include <Swoosh/Segue.h>
#include <Segues/BlackWashFade.h>
#include <Segues/PixelateBlackWashFade.h>

// modals like card cust and battle reward slide in 12px per frame for 10 frames. 60 frames = 1 sec
// modal slide moves 120px in 1/6th of a second
// Per 1 second that is 6*120px in 6*1/6 of a sec = 720px in 1 sec
#define MODAL_SLIDE_PX_PER_SEC 720.0f

using namespace swoosh::types;

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
  this->elapsed = elapsed;
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
        GetController().queuePop<effect>();
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