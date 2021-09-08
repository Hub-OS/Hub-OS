#include "bnHideTimer.h"
#include "bnCharacter.h"
#include "battlescene/bnBattleSceneBase.h"
#include "bnTile.h"
#include "bnAudioResourceManager.h"

HideTimer::HideTimer(std::weak_ptr<Character> owner, double secs) : Component(owner, Component::lifetimes::battlestep) {
  duration = secs;
  elapsed = 0;

  temp = GetOwner()->GetTile();

  respawn = [this]() {
    temp->AddEntity(GetOwner());
  };
}

void HideTimer::OnUpdate(double _elapsed) {
  elapsed += _elapsed;

  if (elapsed >= duration && temp) {
    respawn();
    Eject();
  }
}

void HideTimer::Inject(BattleSceneBase& scene) {
  // temporarily remove from character from play
  auto owner = GetOwner();

  if (owner && temp) {
    // remove then reserve otherwise the API will also clear the reservation
    Entity::ID_t id = owner->GetID();
    temp->RemoveEntityByID(id);
    temp->ReserveEntityByID(id);

    owner->SetTile(nullptr);
  }

  // the component is now injected into the scene's update loop
  // because the character's update loop is only called when they are on the field
  // this way the timer can keep ticking
  scene.Inject(shared_from_base<HideTimer>());
}