#include "bnThunder.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnObstacle.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

Thunder::Thunder(Field* _field, Team _team) : Spell() {
  SetLayer(0);
  field = _field;
  team = _team;
  direction = Direction::NONE;
  deleted = false;
  hit = false;
  texture = TEXTURES.GetTexture(TextureType::SPELL_THUNDER);
  this->elapsed = 0;

  this->slideTime = sf::seconds(1.0f);
  this->timeout = sf::seconds(20.f / 3.f);

  animation = Animation("resources/spells/thunder.animation");
  animation.SetAnimation("DEFAULT");
  animation << Animate::Mode::Loop;

  target = nullptr;
  EnableTileHighlight(false);

  AUDIO.Play(AudioType::THUNDER);
}

Thunder::~Thunder(void) {
}

void Thunder::Update(float _elapsed) {

  if (elapsed > timeout.asSeconds()) {
    this->Delete();
  }

  elapsed += _elapsed;

  setTexture(*texture);
  setScale(2.f, 2.f);
  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y - 30.0f);

  animation.Update(_elapsed, *this);

  // Find target
  if (!target) {
    auto query = [&](Entity* e) {
        return (e->GetTeam() != team && dynamic_cast<Character*>(e) && !dynamic_cast<Obstacle*>(e));
    };

    auto list = field->FindEntities(query);

    for (auto l : list) {
      if (!target) { target = l; }
      else {
        int currentDist = abs(tile->GetX() - l->GetTile()->GetX()) + abs(tile->GetY() - l->GetTile()->GetY());
        int targetDist = abs(tile->GetX() - target->GetTile()->GetX()) + abs(tile->GetY() - target->GetTile()->GetY());

        if (currentDist < targetDist) {
          target = l;
        }
      }
    }
  }

  // Keep moving
  if (!this->isSliding) {
    if (target) {
      if (target->GetTile()) {
        if (target->GetTile()->GetX() < tile->GetX()) {
          direction = Direction::LEFT;
        }
        else if (target->GetTile()->GetX() > tile->GetX()) {
          direction = Direction::RIGHT;
        }
        else if (target->GetTile()->GetY() < tile->GetY()) {
          direction = Direction::UP;
        }
        else if (target->GetTile()->GetY() > tile->GetY()) {
          direction = Direction::DOWN;
        }

        if (target->IsDeleted()) {
          target = nullptr;
        }
      }
    }
    else {
      if (GetTeam() == Team::RED) {
        direction = Direction::RIGHT;
      }
      else {
        direction = Direction::LEFT;
      }
    }

    this->SlideToTile(true);
    this->Move(this->GetDirection());
  }

  tile->AffectEntities(this);

  Entity::Update(_elapsed);
}

bool Thunder::CanMoveTo(Battle::Tile* tile) {
  return true;
}

void Thunder::Attack(Character* _entity) {
  if (_entity && _entity->GetTeam() != this->GetTeam()) {
    Hit::Properties props;
    props.flags = Hit::recoil | Hit::stun;
    props.element = Element::ELEC;
    props.secs = 3;

    if (_entity->Hit(40, props)) {
      this->Delete();
    }
  }
}
