#include <random>
#include <time.h>

#include "bnSharedHitbox.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

SharedHitbox::SharedHitbox(Spell* owner, float duration) : owner(owner), Obstacle(owner->GetField(), owner->GetTeam()) {
  cooldown = duration;
  SetHealth(1);
  ShareTileSpace(true);
  SetHitboxProperties(owner->GetHitboxProperties());
}

SharedHitbox::~SharedHitbox() {
}

void SharedHitbox::OnUpdate(float _elapsed) {
  cooldown -= _elapsed;
  
  tile->AffectEntities(this);
  
  if(cooldown <= 0.f || this->owner->IsDeleted()) {
    this->Delete();
  } else {
	//tile->RemoveEntityByID(this->GetID());
	//tile->AddEntity(*this);
  }
  
  Obstacle::Update(_elapsed);
}

bool SharedHitbox::Move(Direction _direction) {
  return false;
}

const bool SharedHitbox::OnHit(const Hit::Properties props) {
  Character* c = dynamic_cast<Character*>(owner);
	
  if(c) {
	return c->Hit(props); 
  }

  // Passthrough if owner is not an obstacle
  return false;
}

void SharedHitbox::Attack(Character* _entity) {
  if(_entity->GetID() == owner->GetID()) return;
  
  if(owner) {
    owner->Attack(_entity);
  }

  // Remove after first registered hit
  this->Delete();
}

const float SharedHitbox::GetHeight() const {
    if(Character* c = dynamic_cast<Character*>(owner)) { return c->GetHeight(); }
    else { return 0; }
}
