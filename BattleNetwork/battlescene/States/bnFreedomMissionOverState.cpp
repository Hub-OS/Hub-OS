#include "bnFreedomMissionOverState.h"
#include "../bnBattleSceneBase.h"

#include "../../bnPlayer.h"
#include "../../bnField.h"
#include "../../bnCardAction.h"
#include "../../bnAudioResourceManager.h"
#include "../../bnTextureResourceManager.h"

#include <Swoosh/Ease.h>

bool FreedomMissionOverState::IsFinished() {
  return battleEndTimer.getElapsed().asMilliseconds() >= postBattleLength;
}

FreedomMissionOverState::FreedomMissionOverState()
{
  battleEnd = sf::Sprite(*Textures().LoadFromFile(TexturePaths::ENEMY_DELETED));
  battleEnd.setOrigin(battleEnd.getLocalBounds().width / 2.0f, battleEnd.getLocalBounds().height / 2.0f);
  battleEnd.setPosition(sf::Vector2f(240.f, 140.f));
  battleEnd.setScale(2.f, 2.f);
}

void FreedomMissionOverState::onStart(const BattleSceneState*)
{
  if (GetScene().IsPlayerDeleted()) {
    context = Conditions::player_deleted;
  }

  battleEndTimer.reset();
  battleEndTimer.start();

  Audio().StopStream();

  switch (context) {
  case Conditions::player_deleted:
    battleEnd.setTexture(*Textures().LoadFromFile("resources/ui/player_deleted.png"), true);
    break;
  case Conditions::player_won_single_turn:
    battleEnd.setTexture(*Textures().LoadFromFile("resources/ui/single_turn_freedom.png"), true);
    Audio().Stream("resources/loops/enemy_deleted.ogg");
    break;
  case Conditions::player_won_mutliple_turn:
    battleEnd.setTexture(*Textures().LoadFromFile("resources/ui/multi_turn_freedom.png"), true);
    Audio().Stream("resources/loops/enemy_deleted.ogg");
    break;
  case Conditions::player_failed:
    battleEnd.setTexture(*Textures().LoadFromFile("resources/ui/freedom_failed.png"), true);
    break;
  }

  battleEnd.setOrigin(battleEnd.getLocalBounds().width / 2.0f, battleEnd.getLocalBounds().height / 2.0f);

  for (auto p : GetScene().GetAllPlayers()) {
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

void FreedomMissionOverState::onEnd(const BattleSceneState*)
{
}

void FreedomMissionOverState::onUpdate(double elapsed)
{
  battleEndTimer.update(sf::seconds(static_cast<float>(elapsed)));

  // finish whatever animations were happening
  GetScene().GetField()->Update(elapsed);
}

void FreedomMissionOverState::onDraw(sf::RenderTexture& surface)
{
  double battleEndSecs = battleEndTimer.getElapsed().asMilliseconds();
  double scale = swoosh::ease::wideParabola(battleEndSecs, postBattleLength, 2.0);
  battleEnd.setScale(2.f, (float)scale * 2.f);

  surface.draw(battleEnd);
}
