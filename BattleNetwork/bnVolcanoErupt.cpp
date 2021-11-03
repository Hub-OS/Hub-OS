#include "bnVolcanoErupt.h"
#include "bnTile.h"
#include "bnTextureResourceManager.h"

VolcanoErupt::VolcanoErupt() :
  Spell(Team::unknown)
{
  eruptAnim = Animation("resources/tiles/volcano.animation");
  eruptAnim << "ERUPT"  << [this]() {
    this->Erase();
  };;
  
  setScale(2.f, 2.f);
  SetLayer(-1);

  setTexture(Textures().LoadFromFile("resources/tiles/volcano.png"));

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
  GetTile()->AffectEntities(*this);
}

void VolcanoErupt::OnDelete()
{
}

void VolcanoErupt::OnCollision(const std::shared_ptr<Entity>)
{
  // field.lock()->AddEntity(std::make_shared<ParticleImpact>(ParticleImpact::Type::volcano), *GetTile());
}

void VolcanoErupt::Attack(std::shared_ptr<Entity> _entity)
{
  _entity->Hit(GetHitboxProperties());
}
