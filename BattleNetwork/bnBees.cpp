#include "bnBees.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnObstacle.h"
#include "bnParticleImpact.h"
#include "bnHitbox.h"
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
  hitCount = 0;
  attackCooldown = 0.60f;

  animation.Update(0, *this);

  shadow = new SpriteSceneNode();
  shadow->setTexture(LOAD_TEXTURE(MISC_SHADOW));
  shadow->SetLayer(1);
  shadow->setPosition(-8.0f, 20.0f);

  this->AddNode(shadow);

  auto props = GetHitboxProperties();

  // Bees do impact damage only
  props.flags = Hit::impact;

  // Thunder has electric properties
  props.element = Element::WOOD;

  // Attack does 5 units of damage
  props.damage = damage;

  SetHitboxProperties(props);

  if (GetTeam() == Team::RED) {
    SetDirection(Direction::RIGHT);
  }
  else {
    SetDirection(Direction::LEFT);
  }
}

Bees::Bees(Bees & leader) : Spell(leader.GetField(), leader.GetTeam()), damage(leader.damage)
{
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

  target = leader.target;

  turnCount = 0;
  hitCount = 0;
  attackCooldown = 0.60f; // est 2 frames

  animation.Update(0, *this);

  shadow = new SpriteSceneNode();
  shadow->setTexture(LOAD_TEXTURE(MISC_SHADOW));
  shadow->SetLayer(1);
  shadow->setPosition(-12.0f, 18.0f);

  this->AddNode(shadow);

  SetHitboxProperties(leader.GetHitboxProperties());

  this->leader = &leader;

  if (GetTeam() == Team::RED) {
    SetDirection(Direction::RIGHT);
  }
  else {
    SetDirection(Direction::LEFT);
  }
}

Bees::~Bees() {
  delete shadow;
}

void Bees::OnUpdate(float _elapsed) {
  elapsed += _elapsed;

  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y - 60.0f);

  animation.Update(_elapsed, *this);
  
  if (leader && leader->IsDeleted()) {
    leader = nullptr;
    target = nullptr;
  }

  // Find target if we don't have one
  if (!leader && !target) {
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
  else if (leader) {
    // Follow the leader
    target = leader;
  }

  // If sliding is flagged to false, we know we've ended a move
  auto direction = Direction::NONE;
  bool wasMovingVertical   = (GetDirection() == Direction::DOWN || GetDirection() == Direction::UP);
  wasMovingVertical = (wasMovingVertical || GetDirection() == Direction::NONE);

  bool wasMovingHorizontal = (GetDirection() == Direction::LEFT || GetDirection() == Direction::RIGHT);
  wasMovingHorizontal = (wasMovingHorizontal || GetDirection() == Direction::NONE);

  if (!this->IsSliding()) {
    if (target) {
      if (target->GetTile() && turnCount < 2) {
        if (wasMovingVertical || target->GetTile()->GetY() == tile->GetY()) {
          if (target->GetTile()->GetX() < tile->GetX()) {
            direction = Direction::LEFT;
          }
          else if (target->GetTile()->GetX() > tile->GetX()) {
            direction = Direction::RIGHT;
          }
        }
        else if (wasMovingHorizontal && target->GetTile()->GetX() == tile->GetX()) {
          if (target->GetTile()->GetY() < tile->GetY()) {
            direction = Direction::UP;
          }
          else if (target->GetTile()->GetY() > tile->GetY()) {
            direction = Direction::DOWN;
          }
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

    if (direction != this->GetDirection() && direction != Direction::NONE) {
      turnCount++;
      this->SetDirection(direction);
    }

    // Always slide to the tile we're moving to
    this->SlideToTile(true);
    this->Move(this->GetDirection());

    // Did not move and update next tile pointer
    if (!GetNextTile() && GetTile()->IsEdgeTile()) {
      this->Delete();
    }
  }

  if (GetDirection() == Direction::LEFT) {
    this->setScale(2, 2);
  }
  else if (GetDirection() == Direction::RIGHT) {
    this->setScale(-2, 2);
  }

  // Always affect the tile we're occupying
  this->GetTile()->AffectEntities(this);

  if (target && GetTile() == target->GetTile() && attackCooldown == 0) {
    // Try to attack 5 times
    attackCooldown = 1.80f; // est 3 frames
    auto hitbox = new Hitbox(GetField(), GetTeam());
    hitbox->SetHitboxProperties(GetHitboxProperties());
    hitbox->AddCallback([this](Character* entity) {
      // all other hitbox events will be ignored after 5 hits
      if (hitCount < 5) {
        hitCount++;
        AUDIO.Play(AudioType::HURT, AudioPriority::HIGH);
        auto fx = new ParticleImpact(ParticleImpact::Type::GREEN);
        entity->GetField()->AddEntity(*fx, *entity->GetTile());
        fx->SetHeight(entity->GetHeight() / 2.0f);
      }
    });
    GetField()->AddEntity(*hitbox, *GetTile());
    dropped.push_back(hitbox);
  }

  attackCooldown = std::max(attackCooldown - (float)elapsed, 0.0f);
 
  if(hitCount >= 5) {
    // Mark us for deletion
    this->Delete();
  }
}

bool Bees::CanMoveTo(Battle::Tile* tile) {
  return true;
}

void Bees::Attack(Character* _entity) {
  // If entity was successfully hit
  if (hitCount < 5 && _entity->Hit(GetHitboxProperties())) {
    hitCount++;
    AUDIO.Play(AudioType::HURT);
    auto fx = new ParticleImpact(ParticleImpact::Type::GREEN);
    GetField()->AddEntity(*fx, *GetTile());
    fx->SetHeight(_entity->GetHeight()/2.0f);
  }
}

void Bees::OnDelete()
{
  for (auto iter = dropped.begin(); iter != dropped.end(); iter++) {
    (*iter)->Delete();
  }

  dropped.clear();
}
