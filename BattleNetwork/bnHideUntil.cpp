#include "bnHideUntil.h"
#include "bnCharacter.h"
#include "bnBattleScene.h"
#include "bnTile.h"
#include "bnAudioResourceManager.h"

HideUntil::HideUntil(Character* owner, HideUntil::Callback callback) : callback(callback), Component(owner) {
  this->owner = owner;
  temp = owner->GetTile();
}

void HideUntil::OnUpdate(float _elapsed) {
  if ((callback && callback()) && temp) {
    temp->AddEntity(*this->owner);
    this->scene->Eject(this);
    delete this;
  }
}

void HideUntil::Inject(BattleScene& scene) {
  scene.Inject(this);
  this->scene = &scene;

  // it is safe now to temporarily remove from character from play
  // the component is now injected into the scene's update loop

  if (temp) {
    temp->ReserveEntityByID(owner->GetID());
    temp->RemoveEntityByID(owner->GetID());
    owner->SetTile(nullptr);
  }
}