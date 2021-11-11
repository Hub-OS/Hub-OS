#include <random>
#include <time.h>

#include "bnSharedHitbox.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

SharedHitbox::SharedHitbox(std::weak_ptr<Entity> owner, float duration) : 
  owner(owner), 
  Spell(owner.lock()->GetTeam()) {
  cooldown = duration;
  SetHitboxProperties(owner.lock()->GetHitboxProperties());
  keepAlive = (duration == 0.0f);
}

void SharedHitbox::OnUpdate(double _elapsed) {
  cooldown -= _elapsed;

  tile->AffectEntities(*this);

  if (auto owner = this->owner.lock()) {
    if (owner->IsDeleted() || owner->WillEraseEOF()) {
      Delete();
    }
    else if (!keepAlive && cooldown <= 0.f ) {
      Delete();
    }
  }
  else if (keepAlive) {
    // If we are set to KeepAlive but the owner isn't set, delete
    Delete();
  }
}

void SharedHitbox::Attack(std::shared_ptr<Entity> _entity) {
  if(auto owner = this->owner.lock()) {
    if (_entity->GetID() == owner->GetID()) return;
    owner->Attack(_entity);
  }
}

void SharedHitbox::OnDelete()
{
  Erase();
}

void SharedHitbox::OnSpawn(Battle::Tile& start)
{
  auto onOwnerDelete = [](auto target, auto observer) {
    SharedHitbox& hitbox = dynamic_cast<SharedHitbox&>(*observer);
    hitbox.owner.reset();
  };

  field.lock()->NotifyOnDelete(owner.lock()->GetID(), this->GetID(), onOwnerDelete);
}

const float SharedHitbox::GetHeight() const {
  if(auto c = dynamic_cast<Character*>(owner.lock().get())) { 
    return c->GetHeight(); 
  }
  else { 
    return 0; 
  }
}
