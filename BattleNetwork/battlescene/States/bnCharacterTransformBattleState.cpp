#include "bnCharacterTransformBattleState.h"

#include "../bnBattleSceneBase.h"
#include "../../bnPlayer.h"
#include "../../bnSceneNode.h"
#include "../../bnPaletteSwap.h"
#include "../../bnAudioResourceManager.h"
#include "../../bnTextureResourceManager.h"

bool CharacterTransformBattleState::IsFinished() {
    return isLeavingFormChange;
}

void CharacterTransformBattleState::onStart()
{
  this->isLeavingFormChange = true;
}

void CharacterTransformBattleState::onUpdate(double elapsed) {
  for (TrackedFormData data : tracking) {
    Player* player = nullptr;
    int index = -1;

    std::tie(player, index) = data;

    player->Reveal(); // If flickering, stablizes the sprite for the animation

    // Preserve the original child node states
    auto playerChildNodes = player->GetChildNodes();

    // The list of nodes change when adding the shine overlay and new form overlay nodes
    std::shared_ptr<std::vector<SceneNode*>> originalChildNodes(new std::vector<SceneNode*>(playerChildNodes.size()));
    std::shared_ptr<std::vector<bool>> childShaderUseStates(new std::vector<bool>(playerChildNodes.size()));

    /*for (auto child : playerChildNodes) {
      childShaderUseStates->push_back(child->IsUsingParentShader());
      originalChildNodes->push_back(child);
      child->EnableParentShader(true);
    }*/

    auto paletteSwap = player->GetFirstComponent<PaletteSwap>();

    if (paletteSwap) paletteSwap->Enable(false);

    if (backdropTimer < backdropLength) {
      backdropTimer += elapsed;
    }
    else if (backdropTimer >= backdropLength) {
      auto pos = player->getPosition();
      shine.setPosition(pos.x + 16.0f, pos.y - player->GetHeight() / 4.0f);

      auto onTransform = [this, player, index, states = childShaderUseStates, originals = originalChildNodes]() {
        // The next form has a switch based on health
        // This way dying will cancel the form
        // TODO: make this a separate function that takes in form index or something...
        lastSelectedForm = player->GetHealth() == 0 ? -1 : index;
        player->ActivateFormAt(lastSelectedForm);
        AUDIO.Play(AudioType::SHINE);

        player->SetShader(SHADERS.GetShader(ShaderType::WHITE));

        // Activating the form will add NEW child nodes onto our character
        // TODO: There's got to be a more optimal search than this...
        for (auto child : player->GetChildNodes()) {
          auto it = std::find_if(originals->begin(), originals->end(), [child](auto in) {
            return (child == in);
            });

          if (it == originals->end()) {
            states->push_back(child->IsUsingParentShader());
            originals->push_back(child);
            child->EnableParentShader(true); // Add new overlays to this list and make them temporarily white as well
          }
        }
      };

      auto onFinish = [this, paletteSwap, states = childShaderUseStates, originals = originalChildNodes]() {
        isLeavingFormChange = true;
        if (paletteSwap) paletteSwap->Enable();

        unsigned idx = 0;
        for (auto child : *originals) {
          if (!child) {
            idx++; continue;
          }

          unsigned thisIDX = idx;
          bool enabled = (*states)[idx++];
          child->EnableParentShader(enabled);
          Logger::Logf("Enabling state for child #%i: %s", thisIDX, enabled ? "true" : "false");
        }
      };

      shineAnimation << "SHINE" << Animator::On(10, onTransform) << Animator::On(20, onFinish);
    }
  }
}

CharacterTransformBattleState::CharacterTransformBattleState(std::vector<TrackedFormData> tracking) : tracking(tracking)
{
  shine = sf::Sprite(*LOAD_TEXTURE(MOB_BOSS_SHINE));
  shine.setScale(2.f, 2.f);

  shineAnimation = Animation("resources/mobs/boss_shine.animation");
  shineAnimation.Load();
}
