#include "bnInvis.h"
#include "bnEntity.h"
#include "bnCharacter.h"
#include "bnAudioResourceManager.h"

Invis::Invis(Entity* owner) : Component(owner) {
  duration = sf::seconds(15);
  elapsed = 0;
  ResourceHandle().Audio().Play(AudioType::INVISIBLE);
  defense = new DefenseInvis();
  
  auto character = GetOwnerAs<Character>();
  if (character) {
    character->AddDefenseRule(defense);
  }

}

void Invis::OnUpdate(float _elapsed) {
  if (elapsed >= duration.asSeconds()) {
    GetOwner()->SetAlpha(255);
    GetOwner()->SetPassthrough(false); 

    auto character = GetOwnerAs<Character>();
    if (character) {
      character->RemoveDefenseRule(defense);
    }

    delete defense;
    GetOwner()->FreeComponentByID(GetID());
    delete this; 
  }
  else {
    GetOwner()->SetAlpha(125);
    GetOwner()->SetPassthrough(true);
    elapsed += _elapsed;
  }
}

void Invis::Inject(BattleScene&) {
  // Does not effect battle scene
}