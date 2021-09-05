#include "bnCharacterTransformBattleState.h"

#include "../bnBattleSceneBase.h"
#include "../../bnPlayer.h"
#include "../../bnSceneNode.h"
#include "../../bnAudioResourceManager.h"
#include "../../bnTextureResourceManager.h"

CharacterTransformBattleState::CharacterTransformBattleState(std::vector<std::shared_ptr<TrackedFormData>>& tracking) : tracking(tracking)
{
  shine = sf::Sprite(*LOAD_TEXTURE(MOB_BOSS_SHINE));
  shine.setScale(2.f, 2.f);
}

const bool CharacterTransformBattleState::FadeInBackdrop()
{
  return GetScene().FadeInBackdrop(backdropInc, 1.0, true);
}

const bool CharacterTransformBattleState::FadeOutBackdrop()
{
  return GetScene().FadeOutBackdrop(backdropInc);
}

void CharacterTransformBattleState::UpdateAnimation(double elapsed)
{
  bool allCompleted = true;

  size_t count = 0;
  for (auto& data : tracking) {
    auto& [player, index, complete] = *data;

    if (complete) continue;

    allCompleted = false;

    player->Reveal(); // If flickering, stablizes the sprite for the animation

    // The list of nodes change when adding the shine overlay and new form overlay nodes
    std::shared_ptr<std::vector<SceneNode*>> originals(new std::vector<SceneNode*>());
    SmartShader* originalShader = &player->GetShader();

    // I found out structured bindings cannot be captured...
    auto playerPtr = player;
    int _index = index;


    auto onTransform = [=]
    () {
      // The next form has a switch based on health
      // This way dying will cancel the form
      playerPtr->ClearActionQueue();
      playerPtr->ActivateFormAt(_index);
      playerPtr->MakeActionable();
      playerPtr->setColor(NoopCompositeColor(playerPtr->GetColorMode()));
      
      if (auto animComp = playerPtr->GetFirstComponent<AnimationComponent>()) {
        animComp->Refresh();
      }

      if (playerPtr->GetHealth() == 0 && _index == -1) {
        Audio().Play(AudioType::DEFORM);
      }
      else {
        if (playerPtr == GetScene().GetPlayer().get()) {
          // only client player should remove their index information (e.g. PVP battles)
          auto& widget = GetScene().GetCardSelectWidget();
          widget.LockInPlayerFormSelection();
          widget.ErasePlayerFormOption(_index);
        }

        GetScene().HandleCounterLoss(*playerPtr, false);
        Audio().Play(AudioType::SHINE);
      }

      playerPtr->SetShader(Shaders().GetShader(ShaderType::WHITE));
    };

    bool* completePtr = &complete;
    auto onFinish = [=]
    () {
      playerPtr->RefreshShader();
      *completePtr = true; // set tracking data `complete` to true
    };

    if (shineAnimations[count].GetAnimationString() != "SHINE") {
      shineAnimations[count] << "SHINE";

      if (index == -1) {
        // If decross, turn white immediately
        shineAnimations[count] << Animator::On(1, [=] {      
          playerPtr->SetShader(Shaders().GetShader(ShaderType::WHITE));
          }) << Animator::On(10, onTransform);
      }
      else {
        // Else collect child nodes on frame 10 instead
        // We do not want to duplicate the child node collection
        shineAnimations[count] << Animator::On(10, [onTransform] {
          onTransform();
        });
      }

      // Finish on frame 20
      shineAnimations[count] << Animator::On(20, onFinish);

    } // else, it is already "SHINE" so wait the animation out...

    // update for draw call later
    frameElapsed = elapsed;

    count++;
  }

  if (allCompleted) {
    currState = state::fadeout; // we are done
  }
}

void CharacterTransformBattleState::SkipBackdrop()
{
  skipBackdrop = true;
}

bool CharacterTransformBattleState::IsFinished() {
  return state::fadeout == currState && FadeOutBackdrop();
}

void CharacterTransformBattleState::onStart(const BattleSceneState*) {
  if (skipBackdrop) {
    currState = state::animate;
  }
  else {
    currState = state::fadein;
  }

  skipBackdrop = false; // reset this flag
}

void CharacterTransformBattleState::onUpdate(double elapsed) {
  while (shineAnimations.size() < tracking.size()) {
    Animation animation = Animation("resources/mobs/boss_shine.animation");
    animation.Load();
    shineAnimations.push_back(animation);
  }

  switch (currState) {
  case state::fadein:
    if (FadeInBackdrop()) {
      currState = state::animate;
    }
    break;
  case state::animate:
    UpdateAnimation(elapsed);
    break;
  }
}

void CharacterTransformBattleState::onEnd(const BattleSceneState*)
{
  for (auto&& anims : shineAnimations) {
    anims.SetAnimation(""); // ends the shine anim
  }

  frameElapsed = 0;
}

void CharacterTransformBattleState::onDraw(sf::RenderTexture& surface)
{
  size_t count = 0;
  for (auto data : tracking) {
    auto& [player, index, complete] = *data;

    Animation& anim = shineAnimations[count];
    anim.Update(static_cast<float>(frameElapsed), shine);

    if (index != -1 && !complete) {
      // re-use the shine graphic for all animating player-types 
      const sf::Vector2f pos = player->getPosition();
      shine.setPosition(pos.x, pos.y - (player->GetHeight() / 2.0f));
      surface.draw(shine);
    }

    count++;
  }
}
