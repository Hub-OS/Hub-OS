#include "bnInvis.h"
#include "bnEntity.h"
#include "bnAudioResourceManager.h"

Invis::Invis(Entity* owner) : Component(owner) {
  duration = sf::seconds(15);
  elapsed = 0;
  AUDIO.Play(AudioType::INVISIBLE);

}

void Invis::OnUpdate(float _elapsed) {
  if (elapsed >= duration.asSeconds()) {
    this->GetOwner()->SetAlpha(255);
    this->GetOwner()->SetPassthrough(false); 
    this->GetOwner()->FreeComponentByID(this->GetID());
    delete this; 
  }
  else {
    this->GetOwner()->SetAlpha(125);
    this->GetOwner()->SetPassthrough(true);
    elapsed += _elapsed;
  }
}

void Invis::Inject(BattleScene&) {
  // Does not effect battle scene
}