#include "bnCanodumb.h"
#include "bnCanodumbIdleState.h"
#include "bnCanodumbAttackState.h"
#include "bnExplodeState.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnWave.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnGame.h"
#include "bnDefenseVirusBody.h"

#define RESOURCE_NAME "canodumb"
#define RESOURCE_PATH "resources/mobs/canodumb/canodumb.animation"

Canodumb::Canodumb(Rank _rank)
  :  AI<Canodumb>(this), Character(_rank) {
  SetTeam(Team::blue);
  SetName("Canodumb");
  setTexture(Textures().GetTexture(TextureType::MOB_CANODUMB_ATLAS));
  setScale(2.f, 2.f);
  SetHealth(health);

  //Components setup and load
  animation = CreateComponent<AnimationComponent>(this);
  animation->SetPath(RESOURCE_PATH);
  animation->Load();

  switch (GetRank()) {
  case Rank::_1:
    animation->SetAnimation("IDLE_1");
    health = 60;
    break;
  case Rank::_2:
    animation->SetAnimation("IDLE_2");
    health = 90;
    break;
  case Rank::_3:
    animation->SetAnimation("IDLE_3");
    health = 130;
    break;
  }

  virusBody = new DefenseVirusBody();
  AddDefenseRule(virusBody);

  hitHeight = 60;
}

Canodumb::~Canodumb() {
  if (virusBody) {
    RemoveDefenseRule(virusBody);
    delete virusBody;
    virusBody = nullptr;

  }
}

void Canodumb::OnUpdate(float _elapsed) {
  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y);
  hitHeight = getLocalBounds().height;

  AI<Canodumb>::Update(_elapsed);
}

const float Canodumb::GetHeight() const {
  return (float)hitHeight;
}

void Canodumb::OnDelete() {
  // Explode if health depleted
  ChangeState<ExplodeState<Canodumb>>(2);
}


