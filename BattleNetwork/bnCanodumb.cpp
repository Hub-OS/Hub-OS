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
#include "bnDefenseVirusBody.h"

#define RESOURCE_NAME "canodumb"
#define RESOURCE_PATH "resources/mobs/canodumb/canodumb.animation"

Canodumb::Canodumb(Rank _rank)
  :  AI<Canodumb>(this), AnimatedCharacter(_rank) {
  Entity::team = Team::BLUE;

  setTexture(TEXTURES.GetTexture(TextureType::MOB_CANODUMB_ATLAS));
  setScale(2.f, 2.f);

  this->SetHealth(health);

  //Components setup and load
  this->animationComponent->SetPath(RESOURCE_PATH);
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

  virusBody = new DefenseVirusBody();
  this->AddDefenseRule(virusBody);

  hitHeight = 60;
}

Canodumb::~Canodumb() {
  if (virusBody) {
    this->RemoveDefenseRule(virusBody);
    delete virusBody;
    virusBody = nullptr;

  }
}

void Canodumb::OnUpdate(float _elapsed) {
  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y);
  hitHeight = getLocalBounds().height;

  this->AI<Canodumb>::Update(_elapsed);
}

const bool Canodumb::OnHit(const Hit::Properties props) {
  return true;
}

const float Canodumb::GetHeight() const {
  return (float)hitHeight;
}

void Canodumb::OnDelete() {
  // Explode if health depleted
  this->ChangeState<ExplodeState<Canodumb>>(2);
}


