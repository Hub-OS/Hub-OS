#include "bnBubble.h"
#include "bnBubbleTrap.h"
#include "bnAura.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnSharedHitBox.h"

Bubble::Bubble(Field* _field, Team _team, double speed) : Obstacle(field, team), Spell(field, team) {
  SetLayer(1);
  field = _field;

  health = 1;
  
  auto texture = TEXTURES.GetTexture(TextureType::SPELL_BUBBLE);
  
  setTexture(*texture);
  setScale(2.f, 2.f);

  this->speed = speed;

  this->SetSlideTime(sf::seconds(0.5f / (float)speed));

  animation = Animation("resources/spells/bubble.animation");

  auto onFinish = [this]() { animation << "FLOAT" << Animate::Mode::Loop; };

  // Spawn animation and then turns into "FLOAT" which loops forever
  animation << "INIT" << onFinish;

  AUDIO.Play(AudioType::BUBBLE_SPAWN, AudioPriority::LOWEST);

  EnableTileHighlight(false);
  
  // Bubbles can overlap eachother partially
  ShareTileSpace(true);
}

Bubble::~Bubble() {
}

void Bubble::OnUpdate(float _elapsed) {
  setPosition(GetTile()->getPosition().x + tileOffset.x, GetTile()->getPosition().y + tileOffset.y);

  animation.Update(_elapsed*(float)this->speed, *this);

  // Keep moving
  if (!this->IsSliding() && animation.GetAnimationString() == "FLOAT") {
    if (this->GetTile()->GetX() == 1) {
      if (this->GetTile()->GetY() == 2 && this->GetDirection() == Direction::LEFT) {
        this->Delete();
      }
      else if (this->GetTile()->GetY() == 1) {
        if (this->GetDirection() == Direction::LEFT) {
          this->SetDirection(Direction::DOWN);
        }
        else {
          this->Delete();
        }
      }
      else if (this->GetTile()->GetY() == 3) {
        if (this->GetDirection() == Direction::LEFT) {
          this->SetDirection(Direction::UP);
        }
        else {
          this->Delete();
        }
      }
    }
    else if (this->GetTile()->GetX() == 6) {
      this->Delete();
    }

	// Drop a shared hitbox when moving to prevent player from hopping through
	/*SharedHitBox* shb = new SharedHitBox(this, 2.0f);
	GetField()->AddEntity(*shb, tile->GetX(), tile->GetY());*/

    this->SlideToTile(true);
    this->Move(this->GetDirection());
  }

  GetTile()->AffectEntities(this);

  Entity::Update(_elapsed);
}

bool Bubble::CanMoveTo(Battle::Tile* tile) {
  return true;
}


const bool Bubble::OnHit(const Hit::Properties props) {
  // TODO: Hack. Bubbles keep attacking team mates. Why?
  // if(props.aggressor && props.aggressor->GetTeam() == Team::BLUE) return false;

  if (!hit) {
    hit = true;

    auto onFinish = [this]() { this->Delete(); };
    animation << "POP" << onFinish;
    AUDIO.Play(AudioType::BUBBLE_POP, AudioPriority::LOWEST);

    return true;
  }

  return false;
}

void Bubble::Attack(Character* _entity) {
  // TODO: Hack. Bubbles keep attacking team mates. Why?
  if(_entity->GetTeam() == Team::BLUE) return;

  if (!hit) {

    Obstacle* other = dynamic_cast<Obstacle*>(_entity);
    Component* comp = dynamic_cast<Component*>(_entity);

    if (other) {
      if (other->GetHitboxProperties().aggressor != this->GetHitboxProperties().aggressor) {
        _entity->Hit(this->GetHitboxProperties());

        auto onFinish = [this]() { this->Delete(); };
        animation << "POP" << onFinish;
        AUDIO.Play(AudioType::BUBBLE_POP, AudioPriority::LOWEST);
      }
    }
    else {
      hit = _entity->Hit(this->GetHitboxProperties());
    }

    if (hit) {
      if (_entity->GetFirstComponent<BubbleTrap>() == nullptr && !comp) {
        BubbleTrap* trap = new BubbleTrap(_entity);
        _entity->RegisterComponent(trap);
        GetField()->AddEntity(*trap, GetTile()->GetX(), GetTile()->GetY());
      }

      auto onFinish = [this]() { this->Delete(); };
      animation << "POP" << onFinish;
      AUDIO.Play(AudioType::BUBBLE_POP, AudioPriority::LOWEST);
    }
  }
}
