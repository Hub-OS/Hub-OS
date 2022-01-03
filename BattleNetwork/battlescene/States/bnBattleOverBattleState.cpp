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

  BattleResults& results = GetScene().BattleResultsObj();
  results.runaway = false;

  for (std::shared_ptr<Player> p : GetScene().GetAllPlayers()) {
    std::shared_ptr<AnimationComponent> animComponent = p->GetFirstComponent<AnimationComponent>();

    // If animating, let the animation end to look natural
    if (animComponent) {
      animComponent->OnFinish([p] {
        // NOTE: paranoid cleanup codes ALWAYS cleans up!
        // some attacks use nodes that would be cleanup with End() but overwriting the animation prevents this
        std::set<std::shared_ptr<SceneNode>> ourNodes = p->GetChildNodesWithTag({ Player::BASE_NODE_TAG,Player::FORM_NODE_TAG });
        std::vector<std::shared_ptr<SceneNode>> allNodes = p->GetChildNodes();

        for (std::shared_ptr<SceneNode> node : allNodes) {
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
