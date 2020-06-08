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

  setTexture(TEXTURES.GetTexture(TextureType::SPELL_BEES));
  setScale(2.f, 2.f);

  HighlightTile(Battle::Tile::Highlight::solid);

  elapsed = 0;

  SetSlideTime(sf::seconds(0.50f));

  animation = Animation("resources/spells/spell_bees.animation");
  animation.SetAnimation("DEFAULT");
  animation << Animator::Mode::Loop;

  target = nullptr;

  turnCount = 0;
  hitCount = 0;
  attackCooldown = 0.60f;

  animation.Update(0, getSprite());

  shadow = new SpriteProxyNode();
  shadow->setTexture(LOAD_TEXTURE(MISC_SHADOW));
  shadow->SetLayer(1);
  shadow->setPosition(-8.0f, 20.0f);

  AddNode(shadow);

  auto props = GetHitboxProperties();

  // Bees do impact damage only
  props.flags = Hit::impact;

  // Thunder has electric properties
  props.element = Element::wood;

  // Attack does 5 units of damage
  props.damage = damage;

  SetHitboxProperties(props);

  if (GetTeam() == Team::red) {
    SetDirection(Direction::right);
  }
  else {
    SetDirection(Direction::left);
  }
}

Bees::Bees(Bees & leader) : Spell(leader.GetField(), leader.GetTeam()), damage(leader.damage)
{
  SetLayer(0);

  setTexture(TEXTURES.GetTexture(TextureType::SPELL_BEES));
  setScale(2.f, 2.f);

  HighlightTile(Battle::Tile::Highlight::solid);

  elapsed = 0;

  SetSlideTime(sf::seconds(0.50f));

  animation = Animation("resources/spells/spell_bees.animation");
  animation.SetAnimation("DEFAULT");
  animation << Animator::Mode::Loop;

  target = leader.target;

  turnCount = 0;
  hitCount = 0;
  attackCooldown = 0.60f; // est 2 frames

  animation.Update(0, getSprite());

  shadow = new SpriteProxyNode();
  shadow->setTexture(LOAD_TEXTURE(MISC_SHADOW));
  shadow->SetLayer(1);
  shadow->setPosition(-12.0f, 18.0f);

  AddNode(shadow);

  SetHitboxProperties(leader.GetHitboxProperties());

  this->leader = &leader;

  Entity::RemoveCallback& deleteHandler = this->leader->CreateRemoveCallback();
  deleteHandler.Slot([this]() {
      if (target == this->leader) target = nullptr;
      this->leader = nullptr;
  });

  if (GetTeam() == Team::red) {
    SetDirection(Direction::right);
  }
  else {
    SetDirection(Direction::left);
  }
}

Bees::~Bees() {
  delete shadow;
}

void Bees::OnUpdate(float _elapsed) {
  elapsed += _elapsed;

  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y - 60.0f);

  animation.Update(_elapsed, getSprite());

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
  auto direction = Direction::none;
  bool wasMovingVertical   = (GetDirection() == Direction::down || GetDirection() == Direction::up);
  wasMovingVertical = (wasMovingVertical || GetDirection() == Direction::none);

  bool wasMovingHorizontal = (GetDirection() == Direction::left || GetDirection() == Direction::right);
  wasMovingHorizontal = (wasMovingHorizontal || GetDirection() == Direction::none);

  if (!IsSliding()) {
    if (target) {
      if (target->GetTile() && turnCount < 2) {
        if (wasMovingVertical || target->GetTile()->GetY() == tile->GetY()) {
          if (target->GetTile()->GetX() < tile->GetX()) {
            direction = Direction::left;
          }
          else if (target->GetTile()->GetX() > tile->GetX()) {
            direction = Direction::right;
          }
        }
        else if (wasMovingHorizontal && target->GetTile()->GetX() == tile->GetX()) {
          if (target->GetTile()->GetY() < tile->GetY()) {
            direction = Direction::up;
          }
          else if (target->GetTile()->GetY() > tile->GetY()) {
            direction = Direction::down;
          }
        }
      }
    }
    else {
      // If there are no targets, aimlessly move right or left
      // depending on the team
      if (GetTeam() == Team::red) {
        direction = Direction::right;
      }
      else {
        direction = Direction::left;
      }
    }

    if (direction != GetDirection() && direction != Direction::none) {
      turnCount++;
      SetDirection(direction);
    }

    // Always slide to the tile we're moving to
    SlideToTile(true);
    Move(GetDirection());

    // Did not move and update next tile pointer
    if (!GetNextTile() && GetTile()->IsEdgeTile()) {
      Delete();
    }
  }

  if (GetDirection() == Direction::left) {
    setScale(2, 2);
  }
  else if (GetDirection() == Direction::right) {
    setScale(-2, 2);
  }

  // Always affect the tile we're occupying
  GetTile()->AffectEntities(this);

  if (target && GetTile() == target->GetTile() && attackCooldown == 0) {
    // Try to attack 5 times
    attackCooldown = 1.80f; // est 3 frames
    auto hitbox = new Hitbox(GetField(), GetTeam());
    hitbox->SetHitboxProperties(GetHitboxProperties());
    hitbox->AddCallback([this](Character* entity) {
      // all other hitbox events will be ignored after 5 hits
      if (hitCount < 5) {
        hitCount++;
        Audio().Play(AudioType::HURT, AudioPriority::high);
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
    Delete();
  }
}

bool Bees::CanMoveTo(Battle::Tile* tile) {
  return true;
}

void Bees::Attack(Character* _entity) {
  // If entity was successfully hit
  if (hitCount < 5 && _entity->Hit(GetHitboxProperties())) {
    hitCount++;
    Audio().Play(AudioType::HURT);
    auto fx = new ParticleImpact(ParticleImpact::Type::GREEN);
    GetField()->AddEntity(*fx, *GetTile());
    fx->SetHeight(_entity->GetHeight()/2.0f);
  }
}

void Bees::OnDelete()
{
  dropped.clear();
  Remove();
}
