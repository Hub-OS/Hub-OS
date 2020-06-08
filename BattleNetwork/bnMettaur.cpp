#include "bnMettaur.h"
#include "bnMettaurIdleState.h"
#include "bnMettaurAttackState.h"
#include "bnMettaurMoveState.h"
#include "bnExplodeState.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnWave.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnDefenseVirusBody.h"
#include "bnGame.h"

const std::string RESOURCE_PATH = "resources/mobs/mettaur/mettaur.animation";

Mettaur::Mettaur(Rank _rank)
  :  AI<Mettaur>(this), TurnOrderTrait<Mettaur>(), Character(_rank) {
  name = "Mettaur";
  SetTeam(Team::blue);

  animation = CreateComponent<AnimationComponent>(this);
  animation->SetPath(RESOURCE_PATH);
  animation->Reload();

  if (GetRank() == Rank::SP) {
    SetHealth(200);
    animation->SetPlaybackSpeed(1.2);
    animation->SetAnimation("SP_IDLE");
  }
  else {
    SetHealth(40);
    //Components setup and load
    animation->SetAnimation("IDLE");
  }

  hitHeight = 60;

  setTexture(Textures().GetTexture(TextureType::MOB_METTAUR));

  setScale(2.f, 2.f);

  SetHealth(health);

  animation->OnUpdate(0);

  virusBody = new DefenseVirusBody();
  AddDefenseRule(virusBody);
}

Mettaur::~Mettaur() {
  delete virusBody;
}

void Mettaur::OnDelete() {
  RemoveDefenseRule(virusBody);
  ChangeState<ExplodeState<Mettaur>>();

  RemoveMeFromTurnOrder();
}

void Mettaur::OnUpdate(float _elapsed) {
  setPosition(tile->getPosition().x, tile->getPosition().y);
  setPosition(getPosition() + tileOffset);

  AI<Mettaur>::Update(_elapsed);
}

const bool Mettaur::OnHit(const Hit::Properties props) {
  return true;
}

const float Mettaur::GetHeight() const {
  return hitHeight;
}