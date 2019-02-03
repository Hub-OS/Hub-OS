#include "bnProgsMan.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnWave.h"
#include "bnProgBomb.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnEngine.h"
#include "bnExplodeState.h"

#define RESOURCE_NAME "progsman"
#define RESOURCE_PATH "resources/mobs/progsman/progsman.animation"

ProgsMan::ProgsMan(Rank _rank)
  : animationComponent(this),
    AI<ProgsMan>(this), Character(_rank) {
  name = "ProgsMan";
  Entity::team = Team::BLUE;
  health = 300;
  hitHeight = 64;
  state = MOB_IDLE;
  textureType = TextureType::MOB_PROGSMAN_ATLAS;
  healthUI = new MobHealthUI(this);

  this->ChangeState<ProgsManIdleState>();

  setTexture(*TEXTURES.GetTexture(textureType));
  setScale(2.f, 2.f);

  this->SetHealth(health);

  //Components setup and load
  animationComponent.Setup(RESOURCE_PATH);
  animationComponent.Reload();

  whiteout = SHADERS.GetShader(ShaderType::WHITE);
  stun = SHADERS.GetShader(ShaderType::YELLOW);
}

ProgsMan::~ProgsMan(void) {
}

int* ProgsMan::GetAnimOffset() {
  int* res = new int[2];

  res[0] = 10;
  res[1] = 6;

  return res;
}

void ProgsMan::OnFrameCallback(int frame, std::function<void()> onEnter, std::function<void()> onLeave, bool doOnce) {
  animationComponent.AddCallback(frame, onEnter, onLeave, doOnce);
}

void ProgsMan::Update(float _elapsed) {
  healthUI->Update();
  this->SetShader(nullptr);
  this->RefreshTexture();

  if (_elapsed <= 0) return;

  hitHeight = getLocalBounds().height;

  if (stunCooldown > 0) {
    stunCooldown -= _elapsed;
    healthUI->Update();
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

  this->AI<ProgsMan>::Update(_elapsed);

  // Explode if health depleted
  if (GetHealth() <= 0) {
    this->ChangeState<ProgsManHitState>(); // change animation briefly
    this->ChangeState<ExplodeState<ProgsMan>>(12, 0.75); // freezes animation
    this->LockState();
  }
  else {
    animationComponent.Update(_elapsed);
  }

  Character::Update(_elapsed);
}

void ProgsMan::RefreshTexture() {
  setPosition(tile->getPosition().x + this->tileOffset.x, tile->getPosition().y + this->tileOffset.y);
}

vector<Drawable*> ProgsMan::GetMiscComponents() {
  vector<Drawable*> drawables = vector<Drawable*>();
  drawables.push_back(healthUI);

  return drawables;
}

TextureType ProgsMan::GetTextureType() const {
  return textureType;
}

int ProgsMan::GetHealth() const {
  return health;
}

void ProgsMan::SetHealth(int _health) {
  health = _health;
}

const bool ProgsMan::Hit(int _damage, Hit::Properties props) {
  (health - _damage < 0) ? health = 0 : health -= _damage;
  SetShader(whiteout);

  if ((props.flags & Hit::recoil) == Hit::recoil) {
    this->ChangeState<ProgsManHitState>();
  }

  return health;
}

const float ProgsMan::GetHitHeight() const {
  return hitHeight;
}

void ProgsMan::SetCounterFrame(int frame)
{
  auto onFinish = [&]() { this->ToggleCounter(); };
  auto onNext = [&]() { this->ToggleCounter(false); };
  animationComponent.AddCallback(frame, onFinish, onNext);
}

void ProgsMan::SetAnimation(string _state, std::function<void()> onFinish) {
  state = _state;
  animationComponent.SetAnimation(_state, onFinish);
  animationComponent.Update(0);
}
