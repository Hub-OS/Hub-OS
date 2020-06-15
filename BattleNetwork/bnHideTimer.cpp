#include "bnHideTimer.h"
#include "bnCharacter.h"
#include "bnBattleScene.h"
#include "bnTile.h"
#include "bnAudioResourceManager.h"

HideTimer::HideTimer(Character* owner, double secs) : scene(nullptr), Component(owner) {
  duration = secs;
  elapsed = 0;

  temp = GetOwner()->GetTile();

  respawn = [this, owner]() {
    temp->AddEntity(*owner);
  };
}

void HideTimer::OnUpdate(float _elapsed) {
  if(!scene) return;

  if (!scene->IsCleared() && !scene->IsBattleActive()) {
      return;
  }

  elapsed += _elapsed;

  if (elapsed >= duration && temp) {
    respawn();
    scene->Eject(GetID());
    delete this;
  }
}

void HideTimer::Inject(BattleScene& scene) {
  // temporarily remove from character from play
  if (temp) {
    // remove then reserve otherwise the API will also clear the reservation
    temp->RemoveEntityByID(GetOwner()->GetID());
    temp->ReserveEntityByID(GetOwner()->GetID());

    GetOwner()->SetTile(nullptr);
  }

  // the component is now injected into the scene's update loop
  // because the character's update loop is only called when they are on the field
  // this way the timer can keep ticking
  scene.Inject(this);
  this->scene = &scene;
}