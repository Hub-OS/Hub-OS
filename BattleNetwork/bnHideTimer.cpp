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
  elapsed += _elapsed;

  if (elapsed >= duration.asSeconds() && temp) {
    // Adds entity back to original tile and erases the reserve flag
    temp->AddEntity(*this->owner);
    
    // Eject from scene's update loop
    this->scene->Eject(this);
    delete this;
  }
}

void HideTimer::Inject(BattleScene& scene) {
  scene.Inject(this);
  this->scene = &scene;

  // it is safe now to temporarily remove from character from play
  // the component is now injected into the scene's update loop

  this->GetOwner()->FreeComponentByID(this->GetID());

  if (temp) {
    // Reserve the tile state for this entity
    temp->ReserveEntityByID(owner->GetID());
    
    // Remove the entity from the tile
    temp->RemoveEntityByID(owner->GetID());

    // Effectively removes entity from play
    owner->SetTile(nullptr);
  }
}