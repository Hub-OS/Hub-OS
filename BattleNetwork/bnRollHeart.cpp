#include <random>
#include <time.h>

#include "bnRollHeart.h"
#include "bnParticleHeal.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnPlayer.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

#define RESOURCE_PATH "resources/spells/spell_heart.animation"

RollHeart::RollHeart(Character* user, int _heal) 
  : heal(_heal), 
  user(user),
  Spell(user->GetField() , user->GetTeam())
{
  user->Reveal();
  SetLayer(-10);

  SetPassthrough(true);

  HighlightTile(Battle::Tile::Highlight::solid);

  height = 200;

  setTexture(TEXTURES.LoadTextureFromFile("resources/spells/spell_heart.png"), true);
  animationComponent = CreateComponent<AnimationComponent>(this);
  animationComponent->SetPath(RESOURCE_PATH);
  animationComponent->Reload();
  animationComponent->SetAnimation("HEART");
  Update(0);

  doOnce = true;
}

RollHeart::~RollHeart() {
}

void RollHeart::OnUpdate(float _elapsed) {

  if (tile != nullptr) {
    setPosition(tile->getPosition().x, tile->getPosition().y - height - 10.0f);
  }

  height -= _elapsed * 300.f;
  
  if (height <= 0) height = 0;

  if (height == 0 && doOnce) {
    AUDIO.Play(AudioType::RECOVER);
    doOnce = false;

    Delete();
    user->SetHealth(user->GetHealth() + heal);
    auto healfx = new ParticleHeal();
    user->GetField()->AddEntity(*healfx, tile->GetX(), tile->GetY());
  }
}

bool RollHeart::Move(Direction _direction) {
  return false;
}

void RollHeart::Attack(Character* _entity) {
}

void RollHeart::OnDelete()
{
  Remove();
}
