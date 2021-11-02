#include "bnHitboxSpell.h"
#include "bnTile.h"
#include "bnField.h"

HitboxSpell::HitboxSpell(Team _team, int _damage) : Spell(_team) {
  hit = false;
  damage = _damage;

  auto props = Hit::DefaultProperties;
  props.flags |= Hit::impact;
  props.damage = _damage;
  SetHitboxProperties(props);
}

HitboxSpell::~HitboxSpell() {
}

void HitboxSpell::OnSpawn(Battle::Tile & start)
{

}

void HitboxSpell::OnUpdate(double _elapsed) {
  tile->AffectEntities(*this);

  Delete();
}

void HitboxSpell::Attack(std::shared_ptr<Entity> _entity) {
  if (_entity->Hit(GetHitboxProperties()) && attackCallback) {
    attackCallback(_entity);
  }
}

void HitboxSpell::OnCollision(const std::shared_ptr<Entity> _entity)
{
  if (collisionCallback) {
    collisionCallback(_entity);
  }
}

void HitboxSpell::AddCallback(std::function<void(std::shared_ptr<Entity>)> attackCallback, std::function<void(const std::shared_ptr<Entity>)> collisionCallback)
{
  HitboxSpell::attackCallback = attackCallback;
  HitboxSpell::collisionCallback = collisionCallback;
}

void HitboxSpell::OnDelete()
{
  Erase();
}
