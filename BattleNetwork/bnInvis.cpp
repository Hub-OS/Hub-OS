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

Invis::~Invis()
{
  delete defense;
}

void Invis::OnUpdate(double _elapsed) {
  if (elapsed >= duration.asSeconds()) {
    GetOwner()->SetAlpha(255);
    GetOwner()->SetPassthrough(false); 

    auto character = GetOwnerAs<Character>();
    if (character) {
      character->RemoveDefenseRule(defense);
    }
    GetOwner()->FreeComponentByID(GetID());
  }
  else {
    GetOwner()->SetAlpha(125);
    GetOwner()->SetPassthrough(true);
    elapsed += _elapsed;
  }
}

void Invis::Inject(BattleSceneBase&) {
  // Does not effect battle scene
}