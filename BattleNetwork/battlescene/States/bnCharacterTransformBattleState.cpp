#include "bnCharacterTransformBattleState.h"

#include "../bnBattleSceneBase.h"
#include "../../bnPlayer.h"
#include "../../bnSceneNode.h"
#include "../../bnPaletteSwap.h"
#include "../../bnAudioResourceManager.h"
#include "../../bnTextureResourceManager.h"

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
    std::shared_ptr<std::vector<bool>> states(new std::vector<bool>());
    SmartShader* originalShader = &player->GetShader();
    auto paletteSwap = player->GetFirstComponent<PaletteSwap>();

    // I found out structured bindings cannot be captured...
    auto playerPtr = player;
    int index_ = index;

    auto collectChildNodes = [=]
    () {
      // collect ALL child nodes
      for (auto child : playerPtr->GetChildNodesWithTag({ Player::BASE_NODE_TAG, Player::FORM_NODE_TAG })) {
        states->push_back(child->IsUsingParentShader());
        originals->push_back(child);
        child->EnableParentShader(true);
      }
    };

    auto onTransform = [=]
    () {
      // The next form has a switch based on health
      // This way dying will cancel the form
      lastSelectedForm = playerPtr->GetHealth() == 0 ? -1 : index_;
      playerPtr->ActivateFormAt(lastSelectedForm);

      // Reset the player state
      playerPtr->ChangeState<PlayerControlledState>();

      // Interrupt right away
      playerPtr->Character::Update(0);

      if (lastSelectedForm == -1) {
        Audio().Play(AudioType::DEFORM);
      }
      else {
        auto& widget = GetScene().GetCardSelectWidget();
        widget.LockInPlayerFormSelection();
        widget.ErasePlayerFormOption(lastSelectedForm);
        GetScene().HandleCounterLoss(*playerPtr, false);
        Audio().Play(AudioType::SHINE);
      }

      // Activating the form will add NEW child nodes onto our character
      for (auto child : playerPtr->GetChildNodesWithTag({ Player::FORM_NODE_TAG })) {
        states->push_back(child->IsUsingParentShader());
        originals->push_back(child);
        child->EnableParentShader(true); // Add new overlays to this list and make them temporarily white as well
      }

      playerPtr->SetShader(Shaders().GetShader(ShaderType::WHITE));
    };

    bool* completePtr = &complete;
    auto onFinish = [=]
    () {
      // the whiteout shader overwrote the pallette swap shader, apply it again
      if (paletteSwap && paletteSwap->IsEnabled()) {
        paletteSwap->Apply();
      }
      else {
        playerPtr->SetShader(nullptr);
      }

      unsigned idx = 0;

      // undo the forced white shader effect
      for (auto child : *originals) {
        if (!child) {
          idx++; continue;
        }

        bool enabled = (*states)[idx++];
        child->EnableParentShader(enabled);
        // Logger::Logf("Enabling state for child #%i: %s", idx, enabled ? "true" : "false");
      }

      *completePtr = true; // set tracking data `complete` to true
    };

    if (shineAnimations[count].GetAnimationString() != "SHINE") {
      shineAnimations[count] << "SHINE";

      if (index == -1) {
        // If decross, turn white immediately
        shineAnimations[count] << Animator::On(1, [=] {      
          collectChildNodes();
          playerPtr->SetShader(Shaders().GetShader(ShaderType::WHITE));
          }) << Animator::On(10, onTransform);
      }
      else {
        // Else collect child nodes on frame 10 instead
        // We do not want to duplicate the child node collection
        shineAnimations[count] << Animator::On(10, [collectChildNodes, onTransform] {
          collectChildNodes();
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

    if (index != -1) {
      // re-use the shine graphic for all animating player-types 
      auto pos = player->getPosition();
      shine.setPosition(pos.x, pos.y - (player->GetHeight() / 2.0f));

      surface.draw(shine);
    }

    count++;
  }
}

CharacterTransformBattleState::CharacterTransformBattleState(std::vector<std::shared_ptr<TrackedFormData>>& tracking) : tracking(tracking)
{
  shine = sf::Sprite(*LOAD_TEXTURE(MOB_BOSS_SHINE));
  shine.setScale(2.f, 2.f);
}
