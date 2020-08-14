#include "bnBubble.h"
#include "bnBubbleTrap.h"
#include "bnAura.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnSharedHitbox.h"

Bubble::Bubble(Field* _field, Team _team, double speed)
  : popping(false), Obstacle(_field, _team) {
  SetLayer(-100);
  field = _field;

  SetHealth(1);
  SetName("Bubble");

  SetTeam(_team);

  setTexture(TEXTURES.GetTexture(TextureType::SPELL_BUBBLE));
  setScale(2.f, 2.f);

  Bubble::speed = speed;

  SetSlideTime(sf::seconds(0.75f / (float)speed));

  animation = Animation("resources/spells/bubble.animation");

  auto onFinish = [this]() { animation << "FLOAT" << Animator::Mode::Loop; };

  // Spawn animation and then turns into "FLOAT" which loops forever
  animation << "INIT" << onFinish;

  AUDIO.Play(AudioType::BUBBLE_SPAWN, AudioPriority::lowest);

  // Bubbles can overlap eachother partially
  ShareTileSpace(true);

  animation.Update(0, getSprite());

  IgnoreCommonAggressor(true);
}

Bubble::~Bubble() {
}

void Bubble::OnUpdate(float _elapsed) {
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

    SlideToTile(true);
    Move(GetDirection());

    if (!GetNextTile()) {
      Remove(); // Don't pop
    }
  }

  GetTile()->AffectEntities(this);
}

bool Bubble::CanMoveTo(Battle::Tile* tile) {
  return true;
}


const bool Bubble::OnHit(const Hit::Properties props) {
  // don't pop if hit by other bubbles from the same character
  bool sameAggressor = props.aggressor == GetHitboxProperties().aggressor;

  if (!popping && !sameAggressor) {
    popping = true;

    Delete();

    return true;
  }

  return false;
}

void Bubble::OnCollision() {
  Delete();
}

void Bubble::OnDelete()
{
  auto onFinish = [this]() { Remove(); };
  animation << "POP" << onFinish;
  AUDIO.Play(AudioType::BUBBLE_POP, AudioPriority::lowest);
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
    _entity->CreateComponent<BubbleTrap>(_entity);
    Delete();
  }
}
