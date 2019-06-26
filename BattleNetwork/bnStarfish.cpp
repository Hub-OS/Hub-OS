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
  this->SetFloatShoe(true);

  whiteout = SHADERS.GetShader(ShaderType::WHITE);
  stun = SHADERS.GetShader(ShaderType::YELLOW);

  animationComponent.Update(0);
}

Starfish::~Starfish(void) {

}

void Starfish::Update(float _elapsed) {
  if (!hit) {
    this->SetShader(nullptr);
  }
  else {
    SetShader(whiteout);
  }

  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y);

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
    else {
      this->SetShader(nullptr);
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

const bool Starfish::OnHit(const Hit::Properties props) {
  /*if (Character::Hit(_damage, props)) {
    SetShader(whiteout);
    return true;
  }

  return false;*/

  bool result = true;

  if (health - props.damage < 0) {
    health = 0;
  }
  else {
    health -= props.damage;

    // TODO: use resolve system and propagate state information to frame vars
    if (props.element == Element::ELEC) {
      health -= props.damage;
      field->AddEntity(*(new ElementalDamage(field, GetTeam())), tile->GetX(), tile->GetY());
    }

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