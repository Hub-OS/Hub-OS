#include <random>
#include <time.h>

#include "bnSharedHitBox.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

SharedHitBox::SharedHitBox(Spell* owner, float duration) : owner(owner), Obstacle(owner->GetField(), owner->GetTeam()) {
<<<<<<< HEAD
  //field = owner->GetField();
  //team = owner->GetTeam();
  direction = Direction::NONE;
  deleted = false;
  hit = false;
=======
  direction = Direction::NONE;
  deleted = false;
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  cooldown = duration;
  SetHealth(1);
  EnableTileHighlight(false);
  ShareTileSpace(true);
  SetHitboxProperties(owner->GetHitboxProperties());
}

SharedHitBox::~SharedHitBox() {
}

void SharedHitBox::Update(float _elapsed) {
  cooldown -= _elapsed;
  
  tile->AffectEntities(this);
  
  if(cooldown <= 0.f || this->owner->IsDeleted()) {
    this->Delete();
  } else {
<<<<<<< HEAD
	tile->RemoveEntityByID(this->GetID());
	tile->AddEntity(*this);
=======
	//tile->RemoveEntityByID(this->GetID());
	//tile->AddEntity(*this);
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  }
  
  Obstacle::Update(_elapsed);
}

bool SharedHitBox::Move(Direction _direction) {
  return false;
}

const bool SharedHitBox::Hit(Hit::Properties props) {	
  Character* c = dynamic_cast<Character*>(owner);
	
  if(c) {
	return c->Hit(props); 
  }
  
<<<<<<< HEAD
=======
  // Passthrough if owner is not an obstacle
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  return false;
}

void SharedHitBox::Attack(Character* _entity) {
  if(_entity->GetID() == owner->GetID()) return;
  
  if(owner) {
    owner->Attack(_entity);
  }

<<<<<<< HEAD
=======
  // Remove after first registered hit
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  this->Delete();
}
