#include "bnMetalMan.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include "bnEngine.h"
#include "bnNaviExplodeState.h"
#include "bnObstacle.h"
#include "bnHitBox.h"

#define RESOURCE_PATH "resources/mobs/metalman/metalman.animation"

MetalMan::MetalMan(Rank _rank)
  : animationComponent(this),
  AI<MetalMan>(this), Character(_rank) {
  name = "MetalMan";
  this->team = Team::BLUE;

  if (rank == Rank::EX) {
    SetHealth(1300);
  }
  else {
    SetHealth(1000);
  }

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

  movedByStun = false;

  hit = false;
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
  if (!next->ContainsEntityType<Character>() && !next->ContainsEntityType<Obstacle>()) {
    return true;
  }

  return false;
}

void MetalMan::Update(float _elapsed) {
  // TODO: turn all tile altering functions to a queue
  if (movedByStun) { 
    this->Teleport((rand() % 3) + 4, (rand() % 3) + 1); 
    this->AdoptNextTile(); 
    movedByStun = false; 
  }

  // todo: add this in Agent::Update() for ALL agent
  if (this->GetTarget() && this->GetTarget()->IsDeleted()) {
    this->SetTarget(nullptr);
  }

  healthUI->Update(_elapsed);

  if (!hit) {
    this->SetShader(nullptr);
  }

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
    else {
      this->SetShader(nullptr);
    }

    if (GetHealth() > 0) {
      return;
    }
  }

  this->AI<MetalMan>::Update(_elapsed);

  // Explode if health depleted
  if (GetHealth() <= 0) {
    this->ChangeState<NaviExplodeState<MetalMan>>(9, 0.75); // freezes animation
    this->LockState();
  }
  else {
    HitBox* hitbox = new HitBox(field, GetTeam(), 40);
    auto props = hitbox->GetHitboxProperties();
    props.flags |= Hit::impact;
    hitbox->SetHitboxProperties(props);

    field->AddEntity(*hitbox, tile->GetX(), tile->GetY());

    animationComponent.Update(_elapsed);
  }

  Character::Update(_elapsed);

  hit = false;
}

void MetalMan::RefreshTexture() {
  setPosition(tile->getPosition().x + this->tileOffset.x, tile->getPosition().y + this->tileOffset.y);
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

const bool MetalMan::Hit(Hit::Properties props) {
  /*(health - _damage < 0) ? health = 0 : health -= _damage;
  SetShader(whiteout);

  return health;*/

  bool result = true;

  if (health - props.damage < 0) {
    health = 0;
  }
  else {
    health -= props.damage;

    if ((props.flags & Hit::stun) == Hit::stun) {
      SetShader(stun);
      this->stunCooldown = props.secs;

      if (!Teammate(this->GetTile()->GetTeam())) {
        movedByStun = true;
      }
    }
    else {
      SetShader(whiteout);
    }
  }

  hit = result;

  return result;
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
