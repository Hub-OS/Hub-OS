#include "bnHideUntil.h"
#include "bnCharacter.h"
#include "battlescene/bnBattleSceneBase.h"
#include "bnTile.h"
#include "bnAudioResourceManager.h"

HideUntil::HideUntil(std::weak_ptr<Character> owner, HideUntil::Callback callback) 
  : callback(callback), Component(owner) {
  temp = owner.lock()->GetTile();
}

void HideUntil::OnUpdate(double _elapsed) {
  if ((callback && callback()) && temp) {
    if (auto owner = GetOwner()) {
      temp->AddEntity(owner);
      scene->Eject(GetID());
    }
  }
}

void HideUntil::Inject(BattleSceneBase& scene) {
  scene.Inject(shared_from_base<HideUntil>());
  this->scene = &scene;

  // it is safe now to temporarily remove from character from play
  // the component is now injected into the scene's update loop

  if (auto owner = GetOwner(); owner && temp) {
    temp->ReserveEntityByID(owner->GetID());
    temp->RemoveEntityByID(owner->GetID());
    owner->SetTile(nullptr);
  }
}