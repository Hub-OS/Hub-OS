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

RollHeart::RollHeart(CardSummonHandler* _summons, int _heal) : heal(_heal), Spell(_summons->GetCaller()->GetField(), _summons->GetCallerTeam())
{
  summons = _summons;
  caller = summons->GetCaller();
  caller->Reveal();
  SetLayer(-10);

  SetPassthrough(true);

  HighlightTile(Battle::Tile::Highlight::solid);

  height = 200;

  Battle::Tile* _tile = summons->GetCaller()->GetTile();

  field->AddEntity(*this, _tile->GetX(), _tile->GetY());

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
    caller->SetHealth(caller->GetHealth() + heal);
    auto healfx = new ParticleHeal();
    caller->GetField()->AddEntity(*healfx, tile->GetX(), tile->GetY());
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
