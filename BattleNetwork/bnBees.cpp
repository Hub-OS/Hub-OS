#include "bnBees.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnObstacle.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

Bees::Bees(Field* _field, Team _team, int damage) : Spell(_field, _team), damage(damage) {
  SetLayer(0);

  auto texture = TEXTURES.GetTexture(TextureType::SPELL_BEES);
  setTexture(*texture);
  setScale(2.f, 2.f);

  HighlightTile(Battle::Tile::Highlight::solid);

  this->elapsed = 0;

  this->SetSlideTime(sf::seconds(0.50f));

  animation = Animation("resources/spells/spell_bees.animation");
  animation.SetAnimation("DEFAULT");
  animation << Animator::Mode::Loop;

  target = nullptr;

  turnCount = 0;

  animation.Update(0, *this);

  shadow = new SpriteSceneNode();
  shadow->setTexture(LOAD_TEXTURE(MISC_SHADOW));
  shadow->SetLayer(1);
  shadow->setPosition(-4.0f, 0.0f);

  this->AddNode(shadow);

  auto props = GetHitboxProperties();

  // Bees do impact damage only
  props.flags = Hit::impact;

  // Thunder has electric properties
  props.element = Element::WOOD;

  // Attack does 25 units of damage
  props.damage = damage;

  SetHitboxProperties(props);
}

Bees::~Bees() {
  delete shadow;
}

void Bees::OnUpdate(float _elapsed) {
  elapsed += _elapsed;

  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y - 60.0f);

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
        if (turnCount > 0) {
          if (target->GetTile()->GetX() < tile->GetX()) {
            direction = Direction::LEFT;
          }
          else if (target->GetTile()->GetX() > tile->GetX()) {
            direction = Direction::RIGHT;
          }
        }
        
        if (target->GetTile()->GetY() < tile->GetY()) {
          direction = Direction::UP;
        }
        else if (target->GetTile()->GetY() > tile->GetY()) {
          direction = Direction::DOWN;
        }

        // Poll if target is flagged for deletion, remove us
        if (target->IsDeleted()) {
          this->Delete();
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

    if (direction != this->GetDirection()) {
      if (this->GetDirection() == Direction::NONE) {
        this->SetDirection(direction);
      }
      else if (turnCount++ < 2) {
        this->SetDirection(direction);
      }
    }

    // Always slide to the tile we're moving to
    this->SlideToTile(true);
    this->Move(this->GetDirection());

    // Did not move and update next tile pointer
    if (!GetNextTile() && GetTile()->IsEdgeTile()) {
      this->Delete();
    }
  }

  // Always affect the tile we're occupying
  tile->AffectEntities(this);
}

bool Bees::CanMoveTo(Battle::Tile* tile) {
  return true;
}

void Bees::Attack(Character* _entity) {
  // Only attack entities on this tile that are not on our team
  if (_entity && _entity->GetTeam() != this->GetTeam()) {
    // If entity was successfully hit
    if (_entity->Hit(GetHitboxProperties())) {
      // Mark us for deletion
      this->Delete();
      AUDIO.Play(AudioType::HURT);
    }
  }
}
