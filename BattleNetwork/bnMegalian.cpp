#include "bnMegalian.h"
#include "bnMegalianIdleState.h"
#include "bnExplodeState.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnWave.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnEngine.h"
#include "bnAura.h"

const std::string RESOURCE_PATH = "resources/mobs/megalian/megalian.animation";

Megalian::Megalian(Rank _rank)
  : AI<Megalian>(this), Character(_rank) {
  name = "Megalian";
  SetTeam(Team::BLUE);

  animationComponent = new AnimationComponent(this);
  animationComponent->SetPath(RESOURCE_PATH);
  animationComponent->Load();
  this->RegisterComponent(animationComponent);

  auto aura = new Aura(Aura::Type::AURA_100, this);
  aura->Persist(true);

  SetHealth(100);
  //Components setup and load
  animationComponent->SetAnimation("Base1");

  hitHeight = 20;

  setTexture(TEXTURES.GetTexture(TextureType::MOB_MEGALIAN_ATLAS));

  setScale(2.f, 2.f);

  animationComponent->OnUpdate(0);

  head = nullptr;

  this->EnableTilePush(false);
}

Megalian::~Megalian() {
}

void Megalian::OnDelete() {
  this->ChangeState<ExplodeState<Megalian>>(2);
}

void Megalian::OnUpdate(float _elapsed) {
  if (!head) {
    head = new Head(this);
    GetField()->AddEntity(*head, tile->GetX(), tile->GetY());
  }

  setPosition(tile->getPosition().x, tile->getPosition().y);
  setPosition(getPosition() + tileOffset);

  this->AI<Megalian>::Update(_elapsed);
}

const bool Megalian::OnHit(const Hit::Properties props) {
  return true;
}

const float Megalian::GetHeight() const {
  return hitHeight;
}

const bool Megalian::HasAura()
{
  return this->GetFirstComponent<Aura>();
}

bool Megalian::CanMoveTo(Battle::Tile * next)
{
  return true;
}
