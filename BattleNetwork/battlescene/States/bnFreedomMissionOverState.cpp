#include "bnFreedomMissionOverState.h"
#include "../bnBattleSceneBase.h"

#include "../../bnPlayer.h"
#include "../../bnField.h"
#include "../../bnCardAction.h"
#include "../../bnAudioResourceManager.h"
#include "../../bnTextureResourceManager.h"

#include <Swoosh/Ease.h>

FreedomMissionOverState::FreedomMissionOverState() :
  BattleTextIntroState()
{ }

void FreedomMissionOverState::onStart(const BattleSceneState* _)
{
  BattleTextIntroState::onStart(_);

  auto& results = GetScene().BattleResultsObj();
  results.runaway = false;

  if (GetScene().IsPlayerDeleted()) {
    context = Conditions::player_deleted;
  }

  Audio().StopStream();

  switch (context) {
  case Conditions::player_deleted:
    SetIntroText(GetScene().GetLocalPlayer()->GetName() + " deleted!");
    break;
  case Conditions::player_won_single_turn:
    SetIntroText("Single Turn Liberation!");
    Audio().Stream("resources/loops/enemy_deleted.ogg");
    break;
  case Conditions::player_won_mutliple_turn:
    SetIntroText("Liberate success!");
    Audio().Stream("resources/loops/enemy_deleted.ogg");
    break;
  case Conditions::player_failed:
    SetIntroText("Liberation Failed!");
    break;
  }

  for (std::shared_ptr<Player> p : GetScene().GetAllPlayers()) {
    auto animComponent = p->GetFirstComponent<AnimationComponent>();

    // If animating, let the animation end to look natural
    if (animComponent) {
      animComponent->OnFinish([p] {
        if (auto action = p->CurrentCardAction()) {
          action->EndAction();
        }

        // NOTE: paranoid cleanup codes ALWAYS cleans up!
        // some attacks use nodes that would be cleanup with End() but overwriting the animation prevents this
        auto ourNodes = p->GetChildNodesWithTag({ Player::BASE_NODE_TAG,Player::FORM_NODE_TAG });
        auto allNodes = p->GetChildNodes();

        for (auto node : allNodes) {
          auto iter = ourNodes.find(node);
          if (iter == ourNodes.end()) {
            p->RemoveNode(node);
          }
        }
        p->ClearActionQueue();
        p->ChangeState<PlayerIdleState>();
        });
    }
    else {
      // Otherwise force state
      p->ChangeState<PlayerIdleState>();
    }
  }

  GetScene().GetField()->RequestBattleStop();
}


void FreedomMissionOverState::onUpdate(double elapsed)
{
  BattleTextIntroState::onUpdate(elapsed);

  // finish whatever animations were happening
  GetScene().GetField()->Update(elapsed);
}