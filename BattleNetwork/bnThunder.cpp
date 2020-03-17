#include "bnThunder.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnObstacle.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

Thunder::Thunder(Field* _field, Team _team) : Spell(_field, _team) {
  SetLayer(0);

  setTexture(TEXTURES.GetTexture(TextureType::SPELL_THUNDER));
  setScale(2.f, 2.f);

  this->elapsed = 0;

  // Thunder moves from tile to tile in exactly 60 frames
  // The app is clocked at 60 frames a second
  // Therefore thunder slide duration is 1 second
  this->SetSlideTime(sf::seconds(1.0f));
  
  // Thunder is removed in roughly 7 seconds
  this->timeout = sf::seconds(20.f / 3.f);

  animation = Animation("resources/spells/thunder.animation");
  animation.SetAnimation("DEFAULT");
  animation << Animator::Mode::Loop;

  target = nullptr;

  AUDIO.Play(AudioType::THUNDER);

  animation.Update(0, *this);
}

Thunder::~Thunder(void) {
}

void Thunder::OnUpdate(float _elapsed) {

  if (elapsed > timeout.asSeconds()) {
    this->Delete();
  }

  elapsed += _elapsed;
  
  // The origin is the center of the sprite. Raise thunder upwards 15 pixels 
  // (keep in mind scale is 2, e.g. 15 * 2 = 30)
  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y - 30.0f);

  animation.Update(_elapsed, *this);

  // Find target if we don't have one
  if (!target) {
    // Find all characters that are not on our team and not an obstacle
    auto query = [&](Entity* e) {
        return (e->GetTeam() != team && dynamic_cast<Character*>(e) && !dynamic_cast<Obstacle*>(e));
    };

    auto list = field->FindEntities(query);

    for (auto l : list) {
      if (!target) { target = l; }
      else {
        // If the distance to one enemy is shorter than the other, target the shortest enemy path
        int currentDist = abs(tile->GetX() - l->GetTile()->GetX()) + abs(tile->GetY() - l->GetTile()->GetY());
        int targetDist = abs(tile->GetX() - target->GetTile()->GetX()) + abs(tile->GetY() - target->GetTile()->GetY());

        if (currentDist < targetDist) {
          target = l;
        }
      }
    }
  }

  // If sliding is flagged to false, we know we've ended a move
  auto direction = GetDirection();
  if (!this->IsSliding()) {
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

        // Poll if target is flagged for deletion, remove our mark
        if (target->IsDeleted()) {
          target = nullptr;
        }
      }
    }
    else {
      // If there are no targets, aimlessly move right or left
      // depending on the team
      if (GetTeam() == Team::RED) {
        direction = Direction::RIGHT;
      }
      else {
        direction = Direction::LEFT;
      }
    }

    if(direction != this->GetDirection()) {
      this->SetDirection(direction);
    }

    // Always slide to the tile we're moving to
    this->SlideToTile(true);
    this->Move(this->GetDirection());
  }

  // Always affect the tile we're occupying
  tile->AffectEntities(this);
}

bool Thunder::CanMoveTo(Battle::Tile* tile) {
  return true;
}

void Thunder::Attack(Character* _entity) {
  // Only attack entities on this tile that are not on our team
  if (_entity && _entity->GetTeam() != this->GetTeam()) {
    Hit::Properties props;
    
    // Thunder stuns and recoils
    props.flags |= Hit::recoil | Hit::stun;
    
    // Thunder has electric properties
    props.element = Element::ELEC;
    
    // Attack does 40 units of damage
    props.damage = 40;

    // If entity was successfully hit
    if (_entity->Hit(props)) {
      // Mark us for deletion
      this->Delete();
    }
  }
}
