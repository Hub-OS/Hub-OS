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
    temp->AddEntity(*this->owner);
    this->GetOwner()->FreeComponentByID(this->GetID());

    this->scene->Eject(this);
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

    if (temp->GetState() == TileState::BROKEN) {
      temp->SetState(TileState::CRACKED); // TODO: reserve tile hack
    }

    owner->SetTile(nullptr);
  }
}