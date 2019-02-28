#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnAura.h"
#include "bnAuraHealthUI.h"
#include "bnDefenseAura.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/spells/auras.animation"

Aura::Aura(Aura::Type type, Character* owner) : type(type), Character(), Component(owner)
{
  SetLayer(1);
  this->setTexture(*TEXTURES.GetTexture(TextureType::SPELL_AURA));
  this->setScale(2.f, 2.f);
  this->SetTeam(owner->GetTeam());

  healthUI = new AuraHealthUI(this);
  this->RegisterComponent(healthUI);
  //this->AddNode(healthUI);
 
  aura = (sf::Sprite)*this;

  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();

  switch (type) {
  case Aura::Type::AURA_100:
    animation.SetAnimation("AURA_100");
    this->SetHealth(100);
    break;
  case Aura::Type::BARRIER_100:
    animation.SetAnimation("BARRIER_100");
    this->SetHealth(100);
    break;
  }

  this->defense = new DefenseAura();
  owner->AddDefenseRule(defense);

  animation << Animate::Mode::Loop;
  animation.Update(0, *this);
}

void Aura::Inject(BattleScene& bs) {

}

void Aura::Update(float _elapsed) {
  if (GetHealth() == 0) {
    this->TryDelete();
    this->GetOwnerAs<Character>()->RemoveDefenseRule(this->defense);
    this->GetOwner()->FreeComponentByID(this->Component::GetID());
    this->FreeAllComponents();

    return;
  }

 // TODO: who updates this component??
 if (this->GetOwner()->GetTile() == nullptr) {
   this->setScale(0.f, 0.f);
   healthUI->setScale(0.f, 0.f);
   return;
 }
 else {
   // TODO: add Hide and Reveal at SceneNode level
   this->setScale(2.f, 2.f);
   healthUI->setScale(1.f, 1.f);

   this->setPosition(this->tile->getPosition());
 }
 
 if (this->tile != this->GetOwner()->GetTile()) {
   if (this->tile) {
     this->tile->RemoveEntityByID(this->Entity::GetID());
   }

   this->SetTile(this->GetOwner()->GetTile());

   if (tile) {
     this->tile->AddEntity(*this);
   }
 }

 animation.Update(_elapsed, *this);
 Entity::Update(_elapsed);

 if (IsDeleted()) {
   delete this;
 }
}

const Aura::Type Aura::GetAuraType()
{
  return type;
}

const bool Aura::Hit(Hit::Properties props)
{
  std::cout << "aura team: " << (int)this->GetTeam() << std::endl;

  int health = this->GetHealth();

  if (type == Aura::Type::BARRIER_100) {
   health = health - props.damage;
  }

  if (health <= 0) {
    health = 0;
  }

  this->SetHealth(health);

  return true;
}

Aura::~Aura()
{
  std::cout << "Aura deleted" << std::endl;
  delete healthUI;
  delete this->defense;
}