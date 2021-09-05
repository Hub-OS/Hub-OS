#include "bnStarfish.h"
#include "bnStarfishIdleState.h"
#include "bnStarfishAttackState.h"
#include "bnExplodeState.h"
#include "bnElementalDamage.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnWave.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnGame.h"
#include "bnDefenseVirusBody.h"

#define RESOURCE_PATH "resources/mobs/starfish/starfish"

Starfish::Starfish(Rank _rank) : AI<Starfish>(this), Character(_rank) {
  SetName("Starfish");
  team = Team::blue;

  SetElement(Element::aqua);
  animation = CreateComponent<AnimationComponent>(weak_from_this());

  switch (_rank) {
  case Starfish::Rank::SP:
  {
    animation->SetPath(RESOURCE_PATH"2.animation");
    setTexture(Textures().LoadTextureFromFile(RESOURCE_PATH"2_atlas.png"));
    SetHealth(200);
  }
  break;
  default: 
  {
    animation->SetPath(RESOURCE_PATH".animation");
    setTexture(Textures().LoadTextureFromFile(RESOURCE_PATH"_atlas.png"));
    SetHealth(100);
  }
  break;
  }

  animation->Load();
  animation->SetAnimation("IDLE");
  animation->SetPlaybackMode(Animator::Mode::Loop);

  hitHeight = 60;

  setScale(2.f, 2.f);

  SetHealth(health);
  SetFloatShoe(true);

  animation->OnUpdate(0);

  virusBody = std::make_shared<DefenseVirusBody>();
  AddDefenseRule(virusBody);
}

Starfish::~Starfish() {

}

void Starfish::OnUpdate(double _elapsed) {
  AI<Starfish>::Update(_elapsed);
}

const float Starfish::GetHeight() const {
  return hitHeight;
}

void Starfish::OnDelete() {
  if (virusBody) {
    RemoveDefenseRule(virusBody);
    virusBody = nullptr;
  }

  ChangeState<ExplodeState<Starfish>>();
}
