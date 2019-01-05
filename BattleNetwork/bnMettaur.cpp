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
  : animationComponent(this), AI<Mettaur>(this), Character(_rank) {
  this->ChangeState<MettaurIdleState>();
  name = "Mettaur";
  Entity::team = Team::BLUE;

  health = 40;
  textureType = TextureType::MOB_METTAUR_IDLE;

  if (rank == Rank::SP) {
    health = 100;
    textureType = TextureType::MOB_METTAUR_IDLE_BLUE;
    animationComponent.SetPlaybackSpeed(1.2);
  }

  hitHeight = 0;

  healthUI = new MobHealthUI(this);

  setTexture(*TEXTURES.GetTexture(textureType));
  setScale(2.f, 2.f);

  this->SetHealth(health);

  //Components setup and load
  animationComponent.Setup(RESOURCE_PATH);
  animationComponent.Load();

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
  res[0] = 45;  res[1] = 55;

  if (state == MOB_IDLE) {
    res[0] = 35;
    res[1] = 35;
  } else if (state == MOB_ATTACKING) {
    res[0] = 65;
    res[1] = 95;
  } else {
    res[0] = 45;
    res[1] = 55;
  } 

  return res;
}

void Mettaur::Update(float _elapsed) {
  this->SetShader(nullptr);
  this->RefreshTexture();

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

  healthUI->Update();

  if (_elapsed <= 0) return;

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

  Entity::Update(_elapsed);
}

void Mettaur::RefreshTexture() {
  if (state == MOB_IDLE) {
    if (rank == Rank::SP) {
      textureType = TextureType::MOB_METTAUR_IDLE_BLUE;
    }
    else {
      textureType = TextureType::MOB_METTAUR_IDLE;
    }
  } else if (state == MOB_MOVING) {
      textureType = TextureType::MOB_MOVE;
  } else if (state == MOB_ATTACKING) {
    if (rank == Rank::SP) {
      textureType = TextureType::MOB_METTAUR_ATTACK_BLUE;
    }
    else {
      textureType = TextureType::MOB_METTAUR_ATTACK;
    }
  }
  setTexture(*TEXTURES.GetTexture(textureType));

  if (state == MOB_IDLE) {
    setPosition(tile->getPosition().x + tile->GetWidth() / 2.0f - 25.0f, tile->getPosition().y + tile->GetHeight() / 2.0f - 45.0f);
    hitHeight = getLocalBounds().height;
  } else if (state == MOB_MOVING) {
    setPosition(tile->getPosition().x + tile->GetWidth() / 2.0f - 35.0f, tile->getPosition().y + tile->GetHeight() / 2.0f - 60.0f);
  } else if (state == MOB_ATTACKING) {
    setPosition(tile->getPosition().x + tile->GetWidth() / 2.0f - 55.0f, tile->getPosition().y + tile->GetHeight() / 2.0f - 105.0f);
    hitHeight = getLocalBounds().height;
  }
}

vector<Drawable*> Mettaur::GetMiscComponents() {
  vector<Drawable*> drawables = vector<Drawable*>();
  drawables.push_back(healthUI);

  return drawables;
}

void Mettaur::SetAnimation(string _state, std::function<void()> onFinish) {
  state = _state;
  animationComponent.SetAnimation(_state, onFinish);
  animationComponent.Update(0);
}

void Mettaur::SetCounterFrame(int frame)
{
  auto onFinish = [&]() { this->ToggleCounter(); };
  auto onNext = [&]() { this->ToggleCounter(false); };
  animationComponent.AddCallback(frame, onFinish, onNext);
}

TextureType Mettaur::GetTextureType() const {
  return textureType;
}

int Mettaur::GetHealth() const {
  return health;
}

void Mettaur::SetHealth(int _health) {
  health = _health;
}

const bool Mettaur::Hit(int _damage) {
  (health - _damage < 0) ? health = 0 : health -= _damage;
  SetShader(whiteout);

  return health;
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
