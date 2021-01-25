#include "bnVolcanoErupt.h"
#include "bnParticleImpact.h"
#include "bnTextureResourceManager.h"

VolcanoErupt::VolcanoErupt() :
  Spell(Team::unknown)
{
  eruptAnim = Animation("resources/tiles/volcano.animation");
  eruptAnim << "ERUPT"  << [this]() {
    this->Remove();
  };;
  
  setScale(2.f, 2.f);
  SetLayer(-1);

  setTexture(Textures().LoadTextureFromFile("resources/tiles/volcano.png"));

  auto props = GetHitboxProperties();
  props.damage = 50;
  props.flags |= Hit::impact;
  SetHitboxProperties(props);
}

VolcanoErupt::~VolcanoErupt()
{
}

void VolcanoErupt::OnUpdate(double elapsed)
{
  eruptAnim.Update(elapsed, getSprite());
  GetTile()->AffectEntities(this);
}

void VolcanoErupt::OnDelete()
{
}

void VolcanoErupt::OnCollision()
{
  field->AddEntity(*new ParticleImpact(ParticleImpact::Type::volcano), *GetTile());
}

void VolcanoErupt::Attack(Character* _entity)
{
  _entity->Hit(GetHitboxProperties());
}
