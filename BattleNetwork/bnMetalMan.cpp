#include "bnMetalMan.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnEngine.h"
#include "bnExplodeState.h"
#include "bnObstacle.h"

#define RESOURCE_PATH "resources/mobs/metalman/metalman.animation"

MetalMan::MetalMan(Rank _rank)
  : animationComponent(this),
  AI<MetalMan>(this), Character(_rank) {
  name = "MetalMan";
  this->team = Team::BLUE;
  health = 2000;
  hitHeight = 64;
  state = MOB_IDLE;
  textureType = TextureType::MOB_METALMAN_ATLAS;
  healthUI = new MobHealthUI(this);

  this->ChangeState<MetalManIdleState>();

  setTexture(*TEXTURES.GetTexture(textureType));
  setScale(2.f, 2.f);

  this->SetHealth(health);
  this->SetFloatShoe(true);

  //Components setup and load
  animationComponent.Setup(RESOURCE_PATH);
  animationComponent.Reload();
  animationComponent.SetAnimation(MOB_IDLE);

  whiteout = SHADERS.GetShader(ShaderType::WHITE);
  stun = SHADERS.GetShader(ShaderType::YELLOW);

  animationComponent.Update(0);
}

MetalMan::~MetalMan(void) {
}

int* MetalMan::GetAnimOffset() {
  int* res = new int[2];

  res[0] = 10;
  res[1] = 6;

  return res;
}

void MetalMan::OnFrameCallback(int frame, std::function<void()> onEnter, std::function<void()> onLeave, bool doOnce) {
  animationComponent.AddCallback(frame, onEnter, onLeave, doOnce);
}

bool MetalMan::CanMoveTo(Battle::Tile * next)
{
  if (Entity::CanMoveTo(next)) {
    return !next->ContainsEntityType<Obstacle>();
  }

  return false;
}

void MetalMan::Update(float _elapsed) {
  healthUI->Update(_elapsed);
  this->SetShader(nullptr);
  this->RefreshTexture();

  if (_elapsed <= 0) return;

  hitHeight = getLocalBounds().height;

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

  this->AI<MetalMan>::Update(_elapsed);

  // Explode if health depleted
  if (GetHealth() <= 0) {
    this->ChangeState<ExplodeState<MetalMan>>(5, 0.75); // freezes animation
    this->LockState();
  }
  else {
    animationComponent.Update(_elapsed);
  }

  Character::Update(_elapsed);
}

void MetalMan::RefreshTexture() {
  setPosition(tile->getPosition().x + this->tileOffset.x, tile->getPosition().y + this->tileOffset.y);
}

vector<Drawable*> MetalMan::GetMiscComponents() {
  vector<Drawable*> drawables = vector<Drawable*>();
  drawables.push_back(healthUI);

  return drawables;
}

TextureType MetalMan::GetTextureType() const {
  return textureType;
}

int MetalMan::GetHealth() const {
  return health;
}

void MetalMan::SetHealth(int _health) {
  health = _health;
}

const bool MetalMan::Hit(int _damage, Hit::Properties props) {
  (health - _damage < 0) ? health = 0 : health -= _damage;
  SetShader(whiteout);

  return health;
}

const float MetalMan::GetHitHeight() const {
  return hitHeight;
}

void MetalMan::SetCounterFrame(int frame)
{
  auto onFinish = [&]() { this->ToggleCounter(); };
  auto onNext = [&]() { this->ToggleCounter(false); };
  animationComponent.AddCallback(frame, onFinish, onNext);
}

void MetalMan::SetAnimation(string _state, std::function<void()> onFinish) {
  state = _state;
  animationComponent.SetAnimation(_state, onFinish);
  animationComponent.Update(0);
}
