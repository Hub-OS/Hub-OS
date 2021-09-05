#include "bnInvis.h"
#include "bnEntity.h"
#include "bnCharacter.h"
#include "bnAudioResourceManager.h"

Invis::Invis(std::weak_ptr<Entity> owner) : Component(owner) {
  duration = sf::seconds(15);
  elapsed = 0;
  ResourceHandle().Audio().Play(AudioType::INVISIBLE);
  defense = std::make_shared<DefenseInvis>();

  if (auto character = GetOwnerAs<Character>()) {
    character->AddDefenseRule(defense);
  }
}

void Invis::OnUpdate(double _elapsed) {
  auto owner = GetOwner();

  if (elapsed >= duration.asSeconds()) {
    owner->SetAlpha(255);
    owner->SetPassthrough(false);

    if (auto character = GetOwnerAs<Character>()) {
      character->RemoveDefenseRule(defense);
    }

    this->Eject();
  }
  else {
    owner->SetAlpha(125);
    owner->SetPassthrough(true);
    elapsed += _elapsed;
  }
}

void Invis::Inject(BattleSceneBase&) {
  // Does not effect battle scene
}