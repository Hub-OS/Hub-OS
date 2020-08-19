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
#include "bnEngine.h"
#include "bnDefenseVirusBody.h"

#define RESOURCE_PATH "resources/mobs/starfish/starfish.animation"

Starfish::Starfish(Rank _rank)
  : AI<Starfish>(this), Character(_rank) {
  SetName("Starfish");
  team = Team::blue;

  SetElement(Element::aqua);
  SetHealth(100);
  textureType = TextureType::MOB_STARFISH_ATLAS;

  animation = CreateComponent<AnimationComponent>(this);
  animation->SetPath(RESOURCE_PATH);
  animation->Load();
  animation->SetAnimation("IDLE");
  animation->SetPlaybackMode(Animator::Mode::Loop);

  hitHeight = 60;

  setTexture(TEXTURES.GetTexture(textureType));
  setScale(2.f, 2.f);

  SetHealth(health);
  SetFloatShoe(true);

  animation->OnUpdate(0);

  virusBody = new DefenseVirusBody();
  AddDefenseRule(virusBody);
}

Starfish::~Starfish() {

}

void Starfish::OnUpdate(float _elapsed) {
  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y);
  AI<Starfish>::Update(_elapsed);
}

const float Starfish::GetHeight() const {
  return hitHeight;
}

void Starfish::OnDelete() {
  if (virusBody) {
    RemoveDefenseRule(virusBody);
    delete virusBody;
    virusBody = nullptr;
  }

  ChangeState<ExplodeState<Starfish>>();
}
