#include "bnMegalian.h"
#include "bnMegalianIdleState.h"
#include "bnExplodeState.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnWave.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnGame.h"
#include "bnAura.h"

const std::string RESOURCE_PATH = "resources/mobs/megalian/megalian.animation";

Megalian::Megalian(Rank _rank)
  : AI<Megalian>(this), Character(_rank) {
  name = "Megalian";
  SetTeam(Team::blue);

  animationComponent = CreateComponent<AnimationComponent>(weak_from_this());
  animationComponent->SetPath(RESOURCE_PATH);
  animationComponent->Load();

  auto aura = std::make_shared<Aura>(Aura::Type::AURA_100, shared_from_base<Character>());
  aura->Persist(true);

  SetHealth(100);
  //Components setup and load
  animationComponent->SetAnimation("Base1");

  hitHeight = 20;

  setTexture(Textures().GetTexture(TextureType::MOB_MEGALIAN_ATLAS));

  setScale(2.f, 2.f);

  animationComponent->OnUpdate(0);

  head = nullptr;

  EnableTilePush(false);
}

Megalian::~Megalian() {
}

void Megalian::OnDelete() {
  ChangeState<ExplodeState<Megalian>>(2);
}

void Megalian::OnUpdate(double _elapsed) {
  if (!head) {
    head = std::make_shared<Head>(this);
    GetField()->AddEntity(head, tile->GetX(), tile->GetY());
  }

  AI<Megalian>::Update(_elapsed);
}

const float Megalian::GetHeight() const {
  return hitHeight;
}

const bool Megalian::HasAura()
{
  return GetFirstComponent<Aura>() != nullptr;
}

bool Megalian::CanMoveTo(Battle::Tile * next)
{
  return true;
}
