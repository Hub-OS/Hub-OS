#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnAura.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/spells/auras.animation"

Aura::Aura(Aura::Type type, Character* owner) : type(type), Artifact(), Component(owner)
{
  SetLayer(1);
  this->setTexture(*TEXTURES.GetTexture(TextureType::SPELL_AURA));
  this->setScale(2.f, 2.f);
  aura = (sf::Sprite)*this;

  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();

  if (type == Aura::Type::_100) {
    animation.SetAnimation("100");
    animation << Animate::Mode::Loop;
  }

  animation.Update(0, this);
}

void Aura::Inject(BattleScene& bs) {

}

void Aura::Update(float _elapsed) {
 this->setPosition(this->GetOwner()->getPosition());
 
 if (this->tile != this->GetOwner()->GetTile()) {
   this->tile->RemoveEntity(this);
   this->SetTile(this->GetOwner()->GetTile());
   this->tile->AddEntity(this);
 }

 animation.Update(_elapsed, this);
 Entity::Update(_elapsed);
}

const Aura::Type Aura::GetAuraType()
{
  return type;
}

vector<Drawable*> Aura::GetMiscComponents() {
  vector<Drawable*> drawables = vector<Drawable*>();

  return drawables;
}

Aura::~Aura()
{
}