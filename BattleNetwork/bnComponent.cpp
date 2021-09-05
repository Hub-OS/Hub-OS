#include "bnComponent.h"
#include "battlescene/bnBattleSceneBase.h"

long Component::numOfComponents = 0;

Component::Component(std::weak_ptr<Entity> owner, lifetimes lifetime) 
  : owner(owner), lifetime(lifetime) { ID = ++numOfComponents; };

Component::~Component() { ; }

std::shared_ptr<Entity> Component::GetOwner() { return owner.lock(); }

const std::shared_ptr<Entity> Component::GetOwner() const { return owner.lock(); }

const bool Component::Injected() { return Scene() != nullptr; }

BattleSceneBase* Component::Scene() { return scene; }

const Component::lifetimes Component::Lifetime() { return lifetime; }

void Component::FreeOwner() { owner = {}; }

const Component::ID_t Component::GetID() const { return ID; }

void Component::Eject() {
  if (Injected()) {
    Scene()->Eject(GetID());
  }

  auto owner = this->owner.lock();
  if (owner) {
    owner->FreeComponentByID(GetID());
  }
}

void Component::Update(double elapsed) {
  if (Lifetime() == lifetimes::local) {
    this->OnUpdate(elapsed);
  }
  else if (Injected()) {
    this->OnUpdate(elapsed);
  }
}