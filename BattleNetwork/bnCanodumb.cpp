#include "bnCanodumb.h"
#include "bnCanodumbIdleState.h"
#include "bnCanodumbAttackState.h"
#include "bnExplodeState.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnWave.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnEngine.h"

#define RESOURCE_NAME "canodumb"
#define RESOURCE_PATH "resources/mobs/canodumb/canodumb.animation"

Canodumb::Canodumb(Rank _rank)
  :  AI<Canodumb>(this), AnimatedCharacter(_rank) {
  // this->StateChange<CanodumbIdleState>();
  Entity::team = Team::BLUE;
  hitHeight = 25;
  healthUI = new MobHealthUI(this);

  setTexture(*TEXTURES.GetTexture(TextureType::MOB_CANODUMB_ATLAS));
  setScale(2.f, 2.f);

  this->SetHealth(health);

  //Components setup and load
  animationComponent.Setup(RESOURCE_PATH);
  animationComponent.Load();

  switch (GetRank()) {
  case Rank::_1:
    animationComponent.SetAnimation(MOB_CANODUMB_IDLE_1);
    name = "Canodumb";
    health = 60;
    break;
  case Rank::_2:
    animationComponent.SetAnimation(MOB_CANODUMB_IDLE_2);
    name = "Canodumb2";
    health = 90;
    break;
  case Rank::_3:
    animationComponent.SetAnimation(MOB_CANODUMB_IDLE_3);
    name = "Canodumb3";
    health = 130;
    break;
  }

  whiteout = SHADERS.GetShader(ShaderType::WHITE);
  stun = SHADERS.GetShader(ShaderType::YELLOW);

  animationComponent.Update(0);
}

Canodumb::~Canodumb(void) {

}

int* Canodumb::GetAnimOffset() {
  int* res = new int[2];

  res[0] = 15;
  res[1] = 0;

  return res;
}

void Canodumb::Update(float _elapsed) {
  healthUI->Update();
  this->SetShader(nullptr);

  if (_elapsed <= 0) return;

  setPosition(tile->getPosition().x + tile->GetWidth() / 2.0f - 1.0f, tile->getPosition().y + tile->GetHeight() / 2.0f - 10.0f);
  hitHeight = (int)getLocalBounds().height;


  if (stunCooldown > 0) {
    stunCooldown -= _elapsed;
    healthUI->Update();
    Entity::Update(_elapsed);

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

  this->AI<Canodumb>::Update(_elapsed);

  // Explode if health depleted
  if (GetHealth() <= 0) {
    this->ChangeState<ExplodeState<Canodumb>>(3,0.55);
    this->LockState();
  }
  else {
    animationComponent.Update(_elapsed);
  }

  Entity::Update(_elapsed);
}

vector<Drawable*> Canodumb::GetMiscComponents() {
  vector<Drawable*> drawables = vector<Drawable*>();
  drawables.push_back(healthUI);

  return drawables;
}

const bool Canodumb::Hit(int _damage) {
  (health - _damage < 0) ? health = 0 : health -= _damage;
  SetShader(whiteout);

  return health;
}

const float Canodumb::GetHitHeight() const {
  return (float)hitHeight;
}
