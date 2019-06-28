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

#define RESOURCE_PATH "resources/mobs/starfish/starfish.animation"

Starfish::Starfish(Rank _rank)
  : AI<Starfish>(this), AnimatedCharacter(_rank) {
  name = "Starfish";
  this->team = Team::BLUE;

  health = 100;
  textureType = TextureType::MOB_STARFISH_ATLAS;

  animationComponent.Setup(RESOURCE_PATH);
  animationComponent.Reload();

  //Components setup and load
  animationComponent.SetAnimation("IDLE");

  hitHeight = 0;

  healthUI = new MobHealthUI(this);

  setTexture(*TEXTURES.GetTexture(textureType));
  setScale(2.f, 2.f);

  this->SetHealth(health);
  this->SetFloatShoe(true);

  animationComponent.Update(0);
}

Starfish::~Starfish(void) {

}

void Starfish::Update(float _elapsed) {
  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y);

  healthUI->Update(_elapsed);
  Character::Update(_elapsed);

  this->AI<Starfish>::Update(_elapsed);
  animationComponent.Update(_elapsed);
}

const bool Starfish::OnHit(const Hit::Properties props) {
  //  There's no special checks for starfish
  return true;
}

const float Starfish::GetHitHeight() const {
  return hitHeight;
}

void Starfish::OnDelete() {
  this->ChangeState<ExplodeState<Starfish>>();
  this->LockState();
}
