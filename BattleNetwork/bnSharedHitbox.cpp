#include <random>
#include <time.h>

#include "bnSharedHitbox.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

SharedHitbox::SharedHitbox(Spell* owner, float duration) : 
  owner(owner), 
  Spell(owner->GetTeam()) {
  cooldown = duration;
  SetHitboxProperties(owner->GetHitboxProperties());
  keepAlive = (duration == 0.0f);

  Entity::RemoveCallback& deleteHandler = owner->CreateRemoveCallback();
  deleteHandler.Slot([this](Entity*) {
      SharedHitbox::owner = nullptr;
  });
}

SharedHitbox::~SharedHitbox() {
}

void SharedHitbox::OnUpdate(double _elapsed) {
  cooldown -= _elapsed;

  tile->AffectEntities(this);

  if (owner) {
      if (owner->IsDeleted()) {
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

void SharedHitbox::Attack(Character* _entity) {
  if(owner) {
    if (_entity->GetID() == owner->GetID()) return;

    owner->Attack(_entity);
  }

  // Remove after first registered hit
  Delete();
}

void SharedHitbox::OnDelete()
{
  Remove();
}

const float SharedHitbox::GetHeight() const {
    if(Character* c = dynamic_cast<Character*>(owner)) { return c->GetHeight(); }
    else { return 0; }
}
