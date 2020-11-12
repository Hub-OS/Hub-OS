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
  for (auto data : tracking) {
    auto& [player, index, complete] = *data;

    if (complete) continue;

    allCompleted = false;

    player->Reveal(); // If flickering, stablizes the sprite for the animation

    // Preserve the original child node states
    auto playerChildNodes = player->GetChildNodes();

    // The list of nodes change when adding the shine overlay and new form overlay nodes
    std::shared_ptr<std::vector<SceneNode*>> originalChildNodes(new std::vector<SceneNode*>(playerChildNodes.size()));
    std::shared_ptr<std::vector<bool>> childShaderUseStates(new std::vector<bool>(playerChildNodes.size()));

    auto paletteSwap = player->GetFirstComponent<PaletteSwap>();

    if (paletteSwap) paletteSwap->Enable(false);

    auto playerPtr = player;
    int index_ = index; // structured binding cannot be captured...

    auto collectChildNodes = [this, playerPtr, index_, states = childShaderUseStates, originals = originalChildNodes]
    () {
      // collect original child nodes
      for (auto child : playerPtr->GetChildNodes()) {
        states->push_back(child->IsUsingParentShader());
        originals->push_back(child);
        child->EnableParentShader(true);
      }
    };

    auto onTransform = [this, playerPtr, index_, states = childShaderUseStates, originals = originalChildNodes]
    () {
      // The next form has a switch based on health
      // This way dying will cancel the form
      lastSelectedForm = playerPtr->GetHealth() == 0 ? -1 : index_;
      playerPtr->ActivateFormAt(lastSelectedForm);

      if (lastSelectedForm == -1) {
        AUDIO.Play(AudioType::DEFORM);
      }
      else {
        auto& widget = GetScene().GetCardSelectWidget();
        widget.LockInPlayerFormSelection();
        widget.ErasePlayerFormOption(lastSelectedForm);
        GetScene().HandleCounterLoss(*playerPtr);
        AUDIO.Play(AudioType::SHINE);
      }

      // Activating the form will add NEW child nodes onto our character
      for (auto child : playerPtr->GetChildNodes()) {
        auto it = std::find_if(originals->begin(), originals->end(),
          [child](auto in) { return (child == in); }
        );

        // If this is a new node, add it to the list
        if (it == originals->end()) {
          states->push_back(child->IsUsingParentShader());
          originals->push_back(child);
          child->EnableParentShader(true); // Add new overlays to this list and make them temporarily white as well
        }
      }

      playerPtr->SetShader(SHADERS.GetShader(ShaderType::WHITE));
    };

    bool* completePtr = &complete;
    Animation* animPtr = &shineAnimations[count];
    auto onFinish = [this, paletteSwap, states = childShaderUseStates, originals = originalChildNodes, completePtr, animPtr ]
    () {
      if (paletteSwap) paletteSwap->Enable();

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
        shineAnimations[count] << Animator::On(1, [playerPtr, collectChildNodes] {      
          collectChildNodes();
          playerPtr->SetShader(SHADERS.GetShader(ShaderType::WHITE));
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

void CharacterTransformBattleState::onDraw(sf::RenderTexture&)
{
  size_t count = 0;
  for (auto data : tracking) {
    auto& [player, index, complete] = *data;

      Animation& anim = shineAnimations[count];
      anim.Update(static_cast<float>(frameElapsed), shine);

      if (index != -1) {
        // re-use the shine graphic for all animating player-types 
        auto pos = player->getPosition();
        shine.setPosition(pos.x + 16.0f, pos.y - player->GetHeight() / 4.0f);

        ENGINE.Draw(shine, false);
      }

    count++;
  }
}

CharacterTransformBattleState::CharacterTransformBattleState(std::vector<std::shared_ptr<TrackedFormData>>& tracking) : tracking(tracking)
{
  shine = sf::Sprite(*LOAD_TEXTURE(MOB_BOSS_SHINE));
  shine.setScale(2.f, 2.f);
}
