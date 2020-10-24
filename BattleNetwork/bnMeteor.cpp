#include "bnMeteor.h"
#include "bnRingExplosion.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnTile.h"
#include <cmath>
#include <Swoosh/Ease.h>
#include <Swoosh/Game.h>

Meteor::Meteor(Field* _field, Team _team, Battle::Tile* target, int damage, float _duration) : target(target), duration(_duration), Spell(_field, _team) {
  SetLayer(1);

  HighlightTile(Battle::Tile::Highlight::flash);

  setTexture(TEXTURES.GetTexture(TextureType::SPELL_METEOR));

  setScale(0.f, 0.f);

  progress = 0.0f;

  if(target) {
    // Which direction to come down from
    if (GetTeam() == Team::blue) {
      start = sf::Vector2f(target->getPosition().x + 480, target->getPosition().y - 480.0f);
    }
    else if (GetTeam() == Team::red) {
      start = sf::Vector2f(target->getPosition().x + 480, target->getPosition().y - 480.0f);
    }
    else {
      Delete();
    }
  } else {
    Delete();
  }

  auto props = Hit::DefaultProperties;
  props.damage = damage;
  props.flags |= Hit::impact | Hit::flinch | Hit::recoil | Hit::breaking;
  SetHitboxProperties(props);
}

Meteor::~Meteor() {
}

void Meteor::OnUpdate(float _elapsed) {
  if (GetTeam() == Team::blue) {
    setScale(-2.f, 2.f);
    swoosh::game::setOrigin(getSprite(), 1.0, 1.0);
  }
  else {
    setScale(2.f, 2.f);
    swoosh::game::setOrigin(getSprite(), 0.0, 1.0);
  }

  double beta = swoosh::ease::linear(progress, duration, 1.0);

  double posX = (beta * tile->getPosition().x) + ((1.0f - beta) * start.x);
  double posY = (beta * tile->getPosition().y) + ((1.0f - beta) * start.y);

  setPosition((float)posX, (float)posY);

  // When at the end of the arc
  if (beta >= 1.0f) {
    // update tile to target tile
    tile->AffectEntities(this);

    if (tile->GetState() != TileState::empty && tile->GetState() != TileState::broken) {
      ENGINE.GetCamera()->ShakeCamera(5, sf::seconds(0.5));

      field->AddEntity(*(new RingExplosion(field)), *GetTile());
    }

    Delete();
  }

  progress += _elapsed;
}

bool Meteor::Move(Direction _direction) {
  return true;
}

void Meteor::Attack(Character* _entity) {
  _entity->Hit(GetHitboxProperties());
}

void Meteor::OnSpawn(Battle::Tile& start)
{
  AUDIO.Play(AudioType::METEOR, AudioPriority::high);
}

void Meteor::OnDelete()
{
  Remove();
}
