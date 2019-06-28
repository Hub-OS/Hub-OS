#include "bnCanodumb.h"
#include "bnCanodumbIdleState.h"
#include "bnCanodumbAttackState.h"
#include "bnExplodeState.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnWave.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnEngine.h"

#define RESOURCE_NAME "canodumb"
#define RESOURCE_PATH "resources/mobs/canodumb/canodumb.animation"

Canodumb::Canodumb(Rank _rank)
  :  AI<Canodumb>(this), AnimatedCharacter(_rank) {
  Entity::team = Team::BLUE;

  setTexture(*TEXTURES.GetTexture(TextureType::MOB_CANODUMB_ATLAS));
  setScale(2.f, 2.f);

  this->SetHealth(health);

  //Components setup and load
  this->animationComponent->Setup(RESOURCE_PATH);
  this->animationComponent->Load();

  switch (GetRank()) {
  case Rank::_1:
    this->animationComponent->SetAnimation(MOB_CANODUMB_IDLE_1);
    name = "Canodumb";
    health = 60;
    break;
  case Rank::_2:
    this->animationComponent->SetAnimation(MOB_CANODUMB_IDLE_2);
    name = "Canodumb2";
    health = 90;
    break;
  case Rank::_3:
    this->animationComponent->SetAnimation(MOB_CANODUMB_IDLE_3);
    name = "Canodumb3";
    health = 130;
    break;
  }

  // this->animationComponent->Update(0);
}

Canodumb::~Canodumb() {

}

void Canodumb::Update(float _elapsed) {
  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y);
  hitHeight = (int)getLocalBounds().height;

  this->AI<Canodumb>::Update(_elapsed);

  Character::Update(_elapsed);
}

const bool Canodumb::OnHit(const Hit::Properties props) {
  bool result = true;

  if (health - props.damage < 0) {
    health = 0;
  }
  else {
    health -= props.damage;

    if ((props.flags & Hit::stun) == Hit::stun) {
      SetShader(stun);
      this->stunCooldown = props.secs;
    }
    else {
      SetShader(whiteout);
    }
  }

  return result;
}

const float Canodumb::GetHitHeight() const {
  return (float)hitHeight;
}

void Canodumb::OnDelete() {
  // Explode if health depleted
  this->ChangeState<ExplodeState<Canodumb>>(3,0.55);
  this->LockState();
}


