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
<<<<<<< HEAD
  hit = false;
  texture = TEXTURES.GetTexture(TextureType::SPELL_THUNDER);
  this->elapsed = 0;

  this->slideTime = sf::seconds(1.0f);
=======
  
  auto texture = TEXTURES.GetTexture(TextureType::SPELL_THUNDER);
  setTexture(*texture);
  setScale(2.f, 2.f);

  this->elapsed = 0;

  // Thunder moves from tile to tile in exactly 60 frames
  // The app is clocked at 60 frames a second
  // Therefore thunder slide duration is 1 second
  this->slideTime = sf::seconds(1.0f); 
  
  // Thunder is removed in roughly 7 seconds
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
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
<<<<<<< HEAD

  setTexture(*texture);
  setScale(2.f, 2.f);
=======
  
  // The origin is the center of the sprite. Raise thunder upwards 15 pixels 
  // (keep in mind scale is 2, e.g. 15 * 2 = 30)
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y - 30.0f);

  animation.Update(_elapsed, *this);

<<<<<<< HEAD
  // Find target
  if (!target) {
=======
  // Find target if we don't have one
  if (!target) {
    // Find all characters that are not on our team and not an obstacle
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
    auto query = [&](Entity* e) {
        return (e->GetTeam() != team && dynamic_cast<Character*>(e) && !dynamic_cast<Obstacle*>(e));
    };

    auto list = field->FindEntities(query);

    for (auto l : list) {
      if (!target) { target = l; }
      else {
<<<<<<< HEAD
=======
        // If the distance to one enemy is shorter than the other, target the shortest enemy path
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
        int currentDist = abs(tile->GetX() - l->GetTile()->GetX()) + abs(tile->GetY() - l->GetTile()->GetY());
        int targetDist = abs(tile->GetX() - target->GetTile()->GetX()) + abs(tile->GetY() - target->GetTile()->GetY());

        if (currentDist < targetDist) {
          target = l;
        }
      }
    }
  }

<<<<<<< HEAD
  // Keep moving
  if (!this->isSliding) {
=======
  // If sliding is flagged to false, we know we've ended a move 
  if (!this->IsSliding()) {
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
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

<<<<<<< HEAD
=======
        // Poll if target is flagged for deletion, remove our mark
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
        if (target->IsDeleted()) {
          target = nullptr;
        }
      }
    }
    else {
<<<<<<< HEAD
=======
      // If there are no targets, aimlessly move right or left
      // depending on the team
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
      if (GetTeam() == Team::RED) {
        direction = Direction::RIGHT;
      }
      else {
        direction = Direction::LEFT;
      }
    }

<<<<<<< HEAD
=======
    // Always slide to the tile we're moving to
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
    this->SlideToTile(true);
    this->Move(this->GetDirection());
  }

<<<<<<< HEAD
  tile->AffectEntities(this);

=======
  // Always affect the tile we're occupying
  tile->AffectEntities(this);

  // Must call super class to behave correctly
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  Entity::Update(_elapsed);
}

bool Thunder::CanMoveTo(Battle::Tile* tile) {
  return true;
}

void Thunder::Attack(Character* _entity) {
<<<<<<< HEAD
  if (_entity && _entity->GetTeam() != this->GetTeam()) {
    Hit::Properties props;
    props.flags = Hit::recoil | Hit::stun;
    props.element = Element::ELEC;
    props.secs = 3;
    props.damage = 40;

    if (_entity->Hit(props)) {
=======
  // Only attack entities on this tile that are not on our team
  if (_entity && _entity->GetTeam() != this->GetTeam()) {
    Hit::Properties props;
    
    // Thunder stuns and recoils
    props.flags = Hit::recoil | Hit::stun;
    
    // Thunder has electric properties
    props.element = Element::ELEC;
    
    // Stun and recoil last for 3 seconds
    props.secs = 3;
    
    // Attack does 40 units of damage
    props.damage = 40;

    // If entity was successfully hit
    if (_entity->Hit(props)) {
      // Mark us for deletion
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
      this->Delete();
    }
  }
}
