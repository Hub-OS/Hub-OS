#include "bnBubble.h"
#include "bnBubbleTrap.h"
#include "bnAura.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnSharedHitbox.h"

Bubble::Bubble(Team _team,double speed)
  : popping(false), Obstacle(_team) {
  SetLayer(-100);

  SetHealth(1);
  SetName("Bubble");

  SetTeam(_team);

  setTexture(Textures().GetTexture(TextureType::SPELL_BUBBLE));
  setScale(2.f, 2.f);

  Bubble::speed = speed;

  animation = Animation("resources/spells/bubble.animation");

  auto onFinish = [this]() { animation << "FLOAT" << Animator::Mode::Loop; };

  // Spawn animation and then turns into "FLOAT" which loops forever
  animation << "INIT" << onFinish;

  Audio().Play(AudioType::BUBBLE_SPAWN, AudioPriority::lowest);
  
  // Bubbles can overlap eachother partially
  ShareTileSpace(true);

  animation.Update(0, getSprite());

  IgnoreCommonAggressor(true);

  auto props = GetHitboxProperties();
  props.flags |= Hit::bubble;
  SetHitboxProperties(props);
}

Bubble::~Bubble() {
}

void Bubble::OnUpdate(double _elapsed) {
  setPosition(GetTile()->getPosition().x + tileOffset.x, GetTile()->getPosition().y + tileOffset.y);

  animation.Update(_elapsed*(float)speed, getSprite());

  // Keep moving
  if (!IsSliding() && animation.GetAnimationString() == "FLOAT") {
    if (GetTile()->GetX() == 1) {
      if (GetTile()->GetY() == 1) {
        if (GetDirection() == Direction::left) {
          SetDirection(Direction::down);
        }
      }
      else if (GetTile()->GetY() == 3) {
        if (GetDirection() == Direction::left) {
          SetDirection(Direction::up);
        }
      }
    }

    if (!IsSliding()) {
      if (GetTile()->IsEdgeTile()) {
        Remove(); // doesn't make the bubble pop
      }
      else {
        Slide(GetDirection(), frames(45));
      }
    }
  }

  GetTile()->AffectEntities(this);
}

bool Bubble::CanMoveTo(Battle::Tile* tile) {
  return true;
}


const bool Bubble::UnknownTeamResolveCollision(const Spell& other) const {
  Entity* aggro = other.GetHitboxProperties().aggressor;
  bool is_aggro_team = aggro && Teammate(aggro->GetTeam());
  bool is_spell_team = Teammate(other.GetTeam());
  // don't pop if hit by other bubbles from the same character
  return !(is_aggro_team || is_spell_team);
}

void Bubble::OnCollision() {
  Delete();
}

void Bubble::OnDelete()
{
  auto onFinish = [this]() { Remove(); };
  animation << "POP" << onFinish;
  Audio().Play(AudioType::BUBBLE_POP, AudioPriority::lowest);
}

const float Bubble::GetHeight() const
{
  return 80.0f;
}

void Bubble::Attack(Character* _entity) {
  // thsi code looks like it can be rewritten

  if(popping) return;

  Obstacle* other = dynamic_cast<Obstacle*>(_entity);

  if (other) {
    if (other->GetHitboxProperties().aggressor != GetHitboxProperties().aggressor) {
      _entity->Hit(GetHitboxProperties());
      Delete();
    }

    return;
  }
  else {
    popping = popping || _entity->Hit(GetHitboxProperties());
  }

  if (popping) {
    Delete();
  }
}
