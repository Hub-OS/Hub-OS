#include "bnHideTimer.h"
#include "bnCharacter.h"
#include "bnBattleScene.h"
#include "bnTile.h"
#include "bnAudioResourceManager.h"

HideTimer::HideTimer(Character* owner, double secs) : Component(owner) {
  duration = sf::seconds((float)secs);
  elapsed = 0;

  this->owner = owner;
  temp = owner->GetTile();
}

void HideTimer::Update(float _elapsed) {
  if (elapsed >= duration.asSeconds()) {
    temp->AddEntity(*this->owner);
    this->GetOwner()->FreeComponentByID(this->GetID());
    this->scene->Eject(this);
    delete this;
  }

  elapsed += _elapsed;
}

void HideTimer::Inject(BattleScene& scene) {
  scene.Inject(this);
  this->scene = &scene;

  // it is safe now to temporarily remove from character from play
  // the component is now injected into the scene's update loop
  temp->RemoveEntityByID(owner->GetID());
  owner->SetTile(nullptr);
}