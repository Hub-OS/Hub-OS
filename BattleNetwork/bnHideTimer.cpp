#include "bnHideTimer.h"
#include "bnCharacter.h"
#include "bnBattleScene.h"
#include "bnTile.h"
#include "bnAudioResourceManager.h"

HideTimer::HideTimer(Character* owner, double secs) : owner(owner), Component(owner) {
  duration = secs;
  elapsed = 0;

  temp = owner->GetTile();
}

void HideTimer::OnUpdate(float _elapsed) {
  if(!scene) return;

  if (!scene->IsCleared() && !scene->IsBattleActive()) {
      return;
  }

  elapsed += _elapsed;

  if (elapsed >= duration && temp) {
    temp->AddEntity(*owner);
    GetOwner()->FreeComponentByID(GetID());

    scene->Eject(this);
    delete this;
  }
}

void HideTimer::Inject(BattleScene& scene) {
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