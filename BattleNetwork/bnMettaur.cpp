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
#include "bnEngine.h"

const std::string RESOURCE_PATH = "resources/mobs/mettaur/mettaur.animation";

Mettaur::Mettaur(Rank _rank)
  :  AI<Mettaur>(this), TurnOrderTrait<Mettaur>(), AnimatedCharacter(_rank) {
  name = "Mettaur";
  SetTeam(Team::BLUE);

  animationComponent->Setup(RESOURCE_PATH);
  animationComponent->Reload();

  if (GetRank() == Rank::SP) {
    SetHealth(200);
    animationComponent->SetPlaybackSpeed(1.2);
    animationComponent->SetAnimation("SP_IDLE");
  }
  else {
	  SetHealth(40);
    //Components setup and load
    animationComponent->SetAnimation("IDLE");
  }

  hitHeight = 60;

  setTexture(*TEXTURES.GetTexture(TextureType::MOB_METTAUR));

  setScale(2.f, 2.f);

  this->SetHealth(health);

  animationComponent->OnUpdate(0);

  virusBody = new DefenseVirusBody();
  this->AddDefenseRule(virusBody);
}

Mettaur::~Mettaur() {
}

void Mettaur::OnDelete() {
  this->RemoveDefenseRule(virusBody);
  delete virusBody;

  this->ChangeState<ExplodeState<Mettaur>>();
  this->LockState();

  this->RemoveMeFromTurnOrder();
}

void Mettaur::OnUpdate(float _elapsed) {
  setPosition(tile->getPosition().x, tile->getPosition().y);
  setPosition(getPosition() + tileOffset);

  this->AI<Mettaur>::Update(_elapsed);
}

const bool Mettaur::OnHit(const Hit::Properties props) {
    Logger::Log("Mettaur OnHit");

  return true;
}

const float Mettaur::GetHitHeight() const {
  return hitHeight;
}