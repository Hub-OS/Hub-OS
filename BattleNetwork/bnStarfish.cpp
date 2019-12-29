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
  : AI<Starfish>(this), AnimatedCharacter(_rank) {
  this->SetName("Starfish");
  this->team = Team::BLUE;

  this->SetElement(Element::AQUA);
  this->SetHealth(100);
  textureType = TextureType::MOB_STARFISH_ATLAS;

  animationComponent->Setup(RESOURCE_PATH);
  animationComponent->Load();
  animationComponent->SetAnimation("IDLE");
  animationComponent->SetPlaybackMode(Animator::Mode::Loop);

  hitHeight = 60;

  setTexture(*TEXTURES.GetTexture(textureType));
  setScale(2.f, 2.f);

  this->SetHealth(health);
  this->SetFloatShoe(true);

  animationComponent->OnUpdate(0);

  virusBody = new DefenseVirusBody();
  this->AddDefenseRule(virusBody);
}

Starfish::~Starfish() {

}

void Starfish::OnUpdate(float _elapsed) {
  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y);
  this->AI<Starfish>::Update(_elapsed);
}

const bool Starfish::OnHit(const Hit::Properties props) {
  //  There's no special checks for starfish
  return true;
}

const float Starfish::GetHeight() const {
  return hitHeight;
}

void Starfish::OnDelete() {
  if (virusBody) {
    this->RemoveDefenseRule(virusBody);
    delete virusBody;
    virusBody = nullptr;
  }

  this->ChangeState<ExplodeState<Starfish>>();
}
