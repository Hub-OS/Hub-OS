#include "bnFalzar.h"
#include "bnExplodeState.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnDefenseSuperArmor.h"
#include "bnEngine.h"
#include <numeric>

const std::string RESOURCE_PATH = "resources/mobs/falzar/falzar.animation";

Falzar::Falzar(Falzar::Rank rank)
  : BossPatternAI<Falzar>(this),
    Character(rank) {
  SetName("Falzar");
  SetTeam(Team::blue);
  SetAirShoe(true);
  SetFloatShoe(true);

  animation = CreateComponent<AnimationComponent>(this);
  animation->SetPath(RESOURCE_PATH);
  animation->Reload();

  if (GetRank() == Rank::EX) {
    SetHealth(6000);
    animation->SetPlaybackSpeed(1.2);
  }
  else {
    SetHealth(1000);
  }

  animation->SetAnimation("IDLE");

  auto falzarTexture = TEXTURES.LoadTextureFromFile("resources/mobs/falzar/falzar_atlas.png");
  setTexture(falzarTexture);

  setScale(2.f, 2.f);

  animation->OnUpdate(0);

  bossBody = new DefenseSuperArmor();
  AddDefenseRule(bossBody);

  SetHeight(getSprite().getGlobalBounds().height);

  // Add boss pattern code
  AddState<FalzarIdleState>();
  AddState<FalzarMoveState>(3);
  AddState<FalzarRoarState>();
  AddState<FalzarSpinState>(4);
}

Falzar::~Falzar()
{
  delete bossBody;
}


void Falzar::OnDelete() {
  RemoveDefenseRule(bossBody);
  InterruptState<ExplodeState<Falzar>>(50, 1.5f);
}

bool Falzar::CanMoveTo(Battle::Tile* next)
{
  return true;
}

void Falzar::OnUpdate(float _elapsed) {
  BossPatternAI<Falzar>::Update(_elapsed);

  setPosition(tile->getPosition() + tileOffset);
}