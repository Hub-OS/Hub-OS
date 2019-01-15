#include "bnMettaur.h"
#include "bnMettaurIdleState.h"
#include "bnMettaurAttackState.h"
#include "bnMettaurMoveState.h"
#include "bnExplodeState.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnWave.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnEngine.h"

#define RESOURCE_NAME "mettaur"
#define RESOURCE_PATH "resources/mobs/mettaur/mettaur.animation"

#define MOVING_ANIMATION_SPRITES 2
#define MOVING_ANIMATION_WIDTH 32
#define MOVING_ANIMATION_HEIGHT 41

#define IDLE_ANIMATION_WIDTH 54
#define IDLE_ANIMATION_HEIGHT 56

#define HIT_ANIMATION_SPRITES 3
#define HIT_ANIMATION_WIDTH 34
#define HIT_ANIMATION_HEIGHT 31

#define ATTACK_ANIMATION_SPRITES 9
#define ATTACK_ANIMATION_WIDTH 53
#define ATTACK_ANIMATION_HEIGHT 56

vector<int> Mettaur::metIDs = vector<int>();
int Mettaur::currMetIndex = 0;

Mettaur::Mettaur(Rank _rank)
  :  AI<Mettaur>(this), AnimatedCharacter(_rank) {
  //this->ChangeState<MettaurIdleState>();
  name = "Mettaur";
  Entity::team = Team::BLUE;

  health = 40;
  textureType = TextureType::MOB_METTAUR;

  animationComponent.Setup(RESOURCE_PATH);
  animationComponent.Reload();

  if (GetRank() == Rank::SP) {
    health = 100;
    animationComponent.SetPlaybackSpeed(1.2);
    animationComponent.SetAnimation("SP_IDLE");
  }
  else {
    //Components setup and load
    animationComponent.SetAnimation("IDLE");
  }

  hitHeight = 0;

  healthUI = new MobHealthUI(this);

  setTexture(*TEXTURES.GetTexture(textureType));
  setScale(2.f, 2.f);

  this->SetHealth(health);

  whiteout = SHADERS.GetShader(ShaderType::WHITE);
  stun = SHADERS.GetShader(ShaderType::YELLOW);

  metID = (int)Mettaur::metIDs.size();
  Mettaur::metIDs.push_back((int)Mettaur::metIDs.size());

  animationComponent.Update(0);
}

Mettaur::~Mettaur(void) {

}

int* Mettaur::GetAnimOffset() {
  Mettaur* mob = this;

  int* res = new int[2];
  res[0] = 10;  res[1] = 0;

  return res;
}

void Mettaur::Update(float _elapsed) {
  this->SetShader(nullptr);
  this->RefreshTexture();

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

  this->AI<Mettaur>::Update(_elapsed);

  // Explode if health depleted
  if (GetHealth() <= 0) {
    this->ChangeState<ExplodeState<Mettaur>>();
    
    if (Mettaur::metIDs.size() > 0) {
      vector<int>::iterator it = find(Mettaur::metIDs.begin(), Mettaur::metIDs.end(), metID);

      if (it != Mettaur::metIDs.end()) {
        // Remove this mettaur out of rotation...
        Mettaur::currMetIndex++;

        Mettaur::metIDs.erase(it);
        if (Mettaur::currMetIndex >= Mettaur::metIDs.size()) {
          Mettaur::currMetIndex = 0;
        }
      }
    }

    this->LockState();
  } else {
    animationComponent.Update(_elapsed);
  }

  healthUI->Update();

  Character::Update(_elapsed);
}

void Mettaur::RefreshTexture() {
  setPosition(tile->getPosition().x, tile->getPosition().y);

  setPosition(getPosition() + tileOffset);
}

vector<Drawable*> Mettaur::GetMiscComponents() {
  vector<Drawable*> drawables = vector<Drawable*>();
  drawables.push_back(healthUI);

  return drawables;
}

/*void Mettaur::SetAnimation(string _state, std::function<void()> onFinish) {
  state = _state;
  animationComponent.SetAnimation(_state, onFinish);
  animationComponent.Update(0);
}

void Mettaur::SetCounterFrame(int frame)
{
  auto onFinish = [&]() { this->ToggleCounter(); };
  auto onNext = [&]() { this->ToggleCounter(false); };
  animationComponent.AddCallback(frame, onFinish, onNext);
}*/

TextureType Mettaur::GetTextureType() const {
  return textureType;
}

int Mettaur::GetHealth() const {
  return health;
}

void Mettaur::SetHealth(int _health) {
  health = _health;
}

const bool Mettaur::Hit(int _damage, HitProperties props) {

  int previousHealth = health;

  (health - _damage < 0) ? health = 0 : health -= _damage;
  SetShader(whiteout);

  return (health != previousHealth);
}

const float Mettaur::GetHitHeight() const {
  return hitHeight;
}

const bool Mettaur::IsMettaurTurn() const
{
  if(Mettaur::metIDs.size() > 0)
    return (Mettaur::metIDs.at(Mettaur::currMetIndex) == this->metID);

  return false;
}

void Mettaur::NextMettaurTurn() {
  Mettaur::currMetIndex++;

  if (Mettaur::currMetIndex >= Mettaur::metIDs.size()) {
    Mettaur::currMetIndex = 0;
  }
}
