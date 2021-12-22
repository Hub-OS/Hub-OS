#include "bnBattleOverBattleState.h"
#include "../bnBattleSceneBase.h"
#include "../../bnScene.h"
#include "../../bnPlayer.h"
#include "../../bnField.h"
#include "../../bnAnimationComponent.h"

BattleOverBattleState::BattleOverBattleState() : 
  BattleTextIntroState()
{
  SetIntroText("Enemy Deleted!");
}

void BattleOverBattleState::onStart(const BattleSceneState* _)
{
  BattleTextIntroState::onStart(_);

  Audio().Stream("resources/loops/enemy_deleted.ogg");

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


void BattleOverBattleState::onUpdate(double elapsed)
{
  BattleTextIntroState::onUpdate(elapsed);

  // finish whatever animations were happening
  GetScene().GetField()->Update(elapsed);
}
