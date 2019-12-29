#include "bnMetrid.h"
#include "bnMetridIdleState.h"
#include "bnMetridAttackState.h"
#include "bnMetridMoveState.h"
#include "bnExplodeState.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnDefenseVirusBody.h"
#include "bnEngine.h"

const std::string RESOURCE_PATH = "resources/mobs/metrid/metrid.animation";

Metrid::Metrid(Rank _rank)
  : AI<Metrid>(this), TurnOrderTrait<Metrid>(), Character(_rank) {
  name = "Metrid";
  SetTeam(Team::BLUE);

  SetElement(Element::FIRE);

  auto animationComponent = new AnimationComponent(this);
  animationComponent->Setup(RESOURCE_PATH);
  animationComponent->Reload();

  if (GetRank() == Rank::_2) {
    SetHealth(200);
    SetName("Metrod");
    animationComponent->SetPlaybackSpeed(1.2);
    animationComponent->SetAnimation("MOB_IDLE_2");
  }
  else if (GetRank() == Rank::_3) {
    SetName("Metrodo");
    SetHealth(250);
    animationComponent->SetPlaybackSpeed(1.4);
    animationComponent->SetAnimation("MOB_IDLE_3");
  }
  else {
    animationComponent->SetAnimation("MOB_IDLE_1");
    SetHealth(150);
  }

  hitHeight = 60;

  setTexture(*TEXTURES.GetTexture(TextureType::MOB_METRID));
  setScale(2.f, 2.f);
  animationComponent->SetPlaybackMode(Animator::Mode::Loop);

  animationComponent->OnUpdate(0);
  this->RegisterComponent(animationComponent);

  virusBody = new DefenseVirusBody();
  this->AddDefenseRule(virusBody);
}

Metrid::~Metrid() {
}

void Metrid::OnDelete() {
  this->RemoveDefenseRule(virusBody);
  delete virusBody;

  this->ChangeState<ExplodeState<Metrid>>();

  this->RemoveMeFromTurnOrder();
}

void Metrid::OnUpdate(float _elapsed) {
  setPosition(tile->getPosition().x, tile->getPosition().y);
  setPosition(getPosition() + tileOffset);

  this->AI<Metrid>::Update(_elapsed);
}

const bool Metrid::OnHit(const Hit::Properties props) {
  return true;
}

const float Metrid::GetHeight() const {
  return hitHeight;
}