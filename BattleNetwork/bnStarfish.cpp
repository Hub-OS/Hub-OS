#include "bnStarfish.h"
#include "bnStarfishIdleState.h"
#include "bnStarfishAttackState.h"
#include "bnExplodeState.h"
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
  Entity::team = Team::BLUE;

  health = 100;
  hit = false;
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

  whiteout = SHADERS.GetShader(ShaderType::WHITE);
  stun = SHADERS.GetShader(ShaderType::YELLOW);

  animationComponent.Update(0);
}

Starfish::~Starfish(void) {

}

int* Starfish::GetAnimOffset() {
  Starfish* mob = this;

  int* res = new int[2];
  res[0] = 10;  res[1] = 0;

  return res;
}

void Starfish::Update(float _elapsed) {
  if (!hit) {
    this->SetShader(nullptr);
  }
  else {
    SetShader(whiteout);
  }

  this->RefreshTexture();

  if (stunCooldown > 0) {
    stunCooldown -= _elapsed;
    healthUI->Update(_elapsed);
    Character::Update(_elapsed);

    if (stunCooldown <= 0) {
      stunCooldown = 0;
      animationComponent.Update(_elapsed);
    }

    if ((((int)(stunCooldown * 15))) % 2 == 0) {
      this->SetShader(stun);
    }

    if (GetHealth() > 0) {
      return;
    }
  }

  this->AI<Starfish>::Update(_elapsed);

  // Explode if health depleted
  if (GetHealth() <= 0) {
    this->ChangeState<ExplodeState<Starfish>>();
    this->LockState();
  }
  else {
    animationComponent.Update(_elapsed);
  }

  healthUI->Update(_elapsed);

  Character::Update(_elapsed);

  hit = false;
}

void Starfish::RefreshTexture() {
  setPosition(tile->getPosition().x, tile->getPosition().y);

  setPosition(getPosition() + tileOffset);
}

int Starfish::GetHealth() const {
  return health;
}

void Starfish::SetHealth(int _health) {
  health = _health;
}

const bool Starfish::Hit(int _damage, Hit::Properties props) {
  /*if (Character::Hit(_damage, props)) {
    SetShader(whiteout);
    return true;
  }

  return false;*/

  bool result = true;

  if (health - _damage < 0) {
    health = 0;
  }
  else {
    health -= _damage;

    if ((props.flags & Hit::stun) == Hit::stun) {
      SetShader(stun);
      this->stunCooldown = props.secs;
    }
  }

  hit = result;

  return result;
}

const float Starfish::GetHitHeight() const {
  return hitHeight;
}