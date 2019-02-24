#include <random>
#include <time.h>

#include "bnRollHeart.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnPlayer.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

#define RESOURCE_PATH "resources/spells/spell_heart.animation"

RollHeart::RollHeart(ChipSummonHandler* _summons, Player* _player, int _heal) : heal(_heal), Spell()
{
  player = _player;
  summons = _summons;

  player->SetAlpha(255);

  SetPassthrough(true);
  EnableTileHighlight(true);

  field = player->GetField();
  team = player->GetTeam();

  direction = Direction::NONE;
  deleted = false;
  hit = false;
  progress = 0.0f;
  hitHeight = 0.0f;

  Battle::Tile* _tile = player->GetTile();

  this->field->AddEntity(*this, _tile->GetX(), _tile->GetY());

  setTexture(*TEXTURES.LoadTextureFromFile("resources/spells/spell_heart.png"), true);
  animationComponent.Setup(RESOURCE_PATH);
  animationComponent.Reload();
  animationComponent.SetAnimation("HEART");
  this->Update(0);

  doOnce = true;
}

RollHeart::~RollHeart(void) {
}

void RollHeart::Update(float _elapsed) {
  animationComponent.Update(_elapsed);

  if (tile != nullptr) {
    setPosition(tile->getPosition().x + (tile->GetWidth() / 2.0f), tile->getPosition().y - height + (tile->GetHeight() / 2.0f));
  }

  height -= _elapsed * 150.f;
  
  if (height <= 0) height = 0;

  if (height == 0 && doOnce) {
    AUDIO.Play(AudioType::RECOVER);
    doOnce = false;
    this->setColor(sf::Color(255, 255, 255, 0)); // hidden
    player->SetHealth(player->GetHealth() + heal);
    player->SetAnimation("PLAYER_HEAL", [this]() {
      player->SetAnimation("PLAYER_IDLE", [this]() {
        summons->RemoveEntity(this);
      });
    });
  }
}

bool RollHeart::Move(Direction _direction) {
  return true;
}

void RollHeart::Attack(Character* _entity) {
}