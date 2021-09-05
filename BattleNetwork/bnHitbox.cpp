#include <random>
#include <time.h>

#include "bnHitbox.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

Hitbox::Hitbox(Team _team, int _damage) : Spell(_team) {
  hit = false;
  damage = _damage;

  auto props = Hit::DefaultProperties;
  props.flags |= Hit::impact;
  props.damage = _damage;
  SetHitboxProperties(props);
}

Hitbox::~Hitbox() {
}

void Hitbox::OnSpawn(Battle::Tile & start)
{

}

void Hitbox::OnUpdate(double _elapsed) {
  tile->AffectEntities(*this);

  Delete();
}

void Hitbox::Attack(std::shared_ptr<Character> _entity) {
  if (_entity->Hit(GetHitboxProperties()) && attackCallback) {
    attackCallback(_entity);
  }
}

void Hitbox::OnCollision(const std::shared_ptr<Character> _entity)
{
  if (collisionCallback) {
    collisionCallback(_entity);
  }
}

void Hitbox::AddCallback(std::function<void(std::shared_ptr<Character>)> attackCallback, std::function<void(const std::shared_ptr<Character>)> collisionCallback)
{
  Hitbox::attackCallback = attackCallback;
  Hitbox::collisionCallback = collisionCallback;
}

void Hitbox::OnDelete()
{
  Remove();
}
