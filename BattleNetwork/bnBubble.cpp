#include "bnBubble.h"
#include "bnBubbleTrap.h"
#include "bnAura.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnSharedHitbox.h"

Bubble::Bubble(Field* _field, Team _team, double speed) : Obstacle(field, team) {
  SetLayer(-100);
  field = _field;

  SetHealth(1);
  
  SetTeam(team);

  setTexture(TEXTURES.GetTexture(TextureType::SPELL_BUBBLE));
  setScale(2.f, 2.f);

  this->speed = speed;

  this->SetSlideTime(sf::seconds(0.75f / (float)speed));

  animation = Animation("resources/spells/bubble.animation");

  auto onFinish = [this]() { animation << "FLOAT" << Animator::Mode::Loop; };

  // Spawn animation and then turns into "FLOAT" which loops forever
  animation << "INIT" << onFinish;

  AUDIO.Play(AudioType::BUBBLE_SPAWN, AudioPriority::LOWEST);
  
  // Bubbles can overlap eachother partially
  ShareTileSpace(true);

  animation.Update(0, this->getSprite());

  popping = false;
}

Bubble::~Bubble() {
}

void Bubble::OnUpdate(float _elapsed) {
  ResolveFrameBattleDamage();

  setPosition(GetTile()->getPosition().x + tileOffset.x, GetTile()->getPosition().y + tileOffset.y);

  animation.Update(_elapsed*(float)this->speed, this->getSprite());

  // Keep moving
  if (!this->IsSliding() && animation.GetAnimationString() == "FLOAT") {
    if (this->GetTile()->GetX() == 1) {
      if (this->GetTile()->GetY() == 1) {
        if (this->GetDirection() == Direction::LEFT) {
          this->SetDirection(Direction::DOWN);
        }
      }
      else if (this->GetTile()->GetY() == 3) {
        if (this->GetDirection() == Direction::LEFT) {
          this->SetDirection(Direction::UP);
        }
      }
    }

    this->SlideToTile(true);
    this->Move(this->GetDirection());

    if (!this->GetNextTile()) {
      this->Delete();
    }
  }

  GetTile()->AffectEntities(this);
}

bool Bubble::CanMoveTo(Battle::Tile* tile) {
  return true;
}


const bool Bubble::OnHit(const Hit::Properties props) {
  if (!popping) {
    popping = true;

    this->Delete();

    return true;
  }

  return true;
}

void Bubble::OnDelete()
{
  auto onFinish = [this]() { this->Remove(); };
  animation << "POP" << onFinish;
  AUDIO.Play(AudioType::BUBBLE_POP, AudioPriority::LOWEST);
}

const float Bubble::GetHeight() const
{
  return 80.0f;
}

void Bubble::Attack(Character* _entity) {
  if(popping) return;

  Obstacle* other = dynamic_cast<Obstacle*>(_entity);
  Component* comp = dynamic_cast<Component*>(_entity);

  if (other) {
    if (other->GetHitboxProperties().aggressor != this->GetHitboxProperties().aggressor) {
      _entity->Hit(this->GetHitboxProperties());
      this->SetHealth(0);
    }

    return;
  }
  else {
    popping = popping || _entity->Hit(this->GetHitboxProperties());
  }

  if (popping) {
    auto bubble = _entity->GetFirstComponent<BubbleTrap>();
    if (!comp) {
      if (bubble == nullptr) {
        BubbleTrap* trap = new BubbleTrap(_entity);
        _entity->RegisterComponent(trap);
        GetField()->AddEntity(*trap, *this->GetTile());
      }
      else {
        bubble->Pop();
      }
    }

    this->Delete();
  }
}
