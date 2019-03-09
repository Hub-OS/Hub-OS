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
  this->timer = 50; // seconds
  this->setTexture(*TEXTURES.GetTexture(TextureType::SPELL_AURA));
  this->setScale(2.f, 2.f);
  this->SetTeam(owner->GetTeam());
  this->SetFloatShoe(true);
  this->ShareTileSpace(true);

  healthUI = new AuraHealthUI(this);
  this->RegisterComponent(healthUI);

  aura = (sf::Sprite)*this;

  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();

  switch (type) {
  case Aura::Type::AURA_100:
    animation.SetAnimation("AURA_100");
    this->SetHealth(100);
    break;
  case Aura::Type::AURA_200:
    animation.SetAnimation("AURA_200");
    this->SetHealth(200);
    break;
  case Aura::Type::AURA_1000:
    animation.SetAnimation("AURA_1000");
    this->SetHealth(1000);
    break;
  case Aura::Type::BARRIER_100:
    animation.SetAnimation("BARRIER_100");
    this->SetHealth(100);
    break;
  case Aura::Type::BARRIER_200:
    animation.SetAnimation("BARRIER_200");
    this->SetHealth(200);
    break;
  case Aura::Type::BARRIER_500:
    animation.SetAnimation("BARRIER_500");
    this->SetHealth(500);
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
  timer -= _elapsed;

  this->SlideToTile(false);

  if (GetHealth() == 0 || timer <= 0.0) {
    this->TryDelete();
    this->GetOwnerAs<Character>()->RemoveDefenseRule(this->defense);
    this->GetOwner()->FreeComponentByID(this->Component::GetID());
    this->FreeAllComponents();
    Entity::Update(_elapsed);

    return;
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

 if (this->GetOwner()->GetTile() == nullptr) {
   this->Hide();
   healthUI->Hide();
   return;
 }
 else {
   this->Reveal();
   healthUI->Reveal();

   this->setPosition(this->tile->getPosition());
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
  int health = this->GetHealth();

  if (type >= Aura::Type::BARRIER_100) {
   health = health - props.damage;
  }
  else if (health <= props.damage) {
    health = 0;
  }

  if (health <= 0) {
    health = 0;
  }

  this->SetHealth(health);

  return true;
}

Aura::~Aura()
{
  delete healthUI;
  delete this->defense;
}