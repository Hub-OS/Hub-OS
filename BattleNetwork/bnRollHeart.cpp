#include <random>
#include <time.h>

#include "bnRollHeart.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnPlayer.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

#define RESOURCE_PATH "resources/spells/spell_heart.animation"

RollHeart::RollHeart(CardSummonHandler* _summons, int _heal) : heal(_heal), Spell(_summons->GetCaller()->GetField(), _summons->GetCallerTeam())
{
  summons = _summons;

  summons->GetCaller()->Reveal();

  SetPassthrough(true);

  this->HighlightTile(Battle::Tile::Highlight::solid);

  height = 200;

  Battle::Tile* _tile = summons->GetCaller()->GetTile();

  this->field->AddEntity(*this, _tile->GetX(), _tile->GetY());

  setTexture(TEXTURES.LoadTextureFromFile("resources/spells/spell_heart.png"), true);
  animationComponent = new AnimationComponent(this);
  this->RegisterComponent(animationComponent);
  animationComponent->Setup(RESOURCE_PATH);
  animationComponent->Reload();
  animationComponent->SetAnimation("HEART");
  this->Update(0);

  doOnce = true;
}

RollHeart::~RollHeart() {
}

void RollHeart::OnUpdate(float _elapsed) {

  if (tile != nullptr) {
    setPosition(tile->getPosition().x + (tile->GetWidth() / 2.0f), tile->getPosition().y - height + (tile->GetHeight() / 2.0f));
  }

  height -= _elapsed * 150.f;
  
  if (height <= 0) height = 0;

  if (height == 0 && doOnce) {
    AUDIO.Play(AudioType::RECOVER);
    doOnce = false;

    this->setColor(sf::Color(255, 255, 255, 0)); // hide
    caller->SetHealth(caller->GetHealth() + heal);
    
    /*player->SetAnimation("PLAYER_HEAL", [this]() {
      player->SetAnimation("PLAYER_IDLE", [this]() {
        summons->RemoveEntity(this);
      });
    });*/
  }
}

bool RollHeart::Move(Direction _direction) {
  return false;
}

void RollHeart::Attack(Character* _entity) {
}