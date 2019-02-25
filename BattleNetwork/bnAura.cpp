#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnAura.h"
#include "bnDefenseAura.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/spells/auras.animation"

Aura::Aura(Aura::Type type, Character* owner) : type(type), Character(), Component(owner)
{
  SetLayer(1);
  this->setTexture(*TEXTURES.GetTexture(TextureType::SPELL_AURA));
  this->setScale(2.f, 2.f);
  this->SetTeam(owner->GetTeam());

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
 this->setPosition(this->GetOwner()->getPosition());
 
 if (this->tile != this->GetOwner()->GetTile()) {
   if (this->tile) {
     this->tile->RemoveEntityByID(this->GetID());
   }

   this->SetTile(this->GetOwner()->GetTile());
   this->tile->AddEntity(*this);
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

const bool Aura::Hit(int _damage, Hit::Properties props)
{
  int health = this->GetHealth();

  if (type == Aura::Type::BARRIER_100) {
   health = health - _damage;
  }


  if (health < 0) { health = 0; this->TryDelete(); /*this->GetOwner()->FreeComponent(*this);*/ }

  this->SetHealth(health);

  return health != this->GetHealth();
}

Aura::~Aura()
{
  std::cout << "Aura deleted" << std::endl;
  this->GetOwnerAs<Character>()->RemoveDefenseRule(this->defense);

  delete this->defense;
}