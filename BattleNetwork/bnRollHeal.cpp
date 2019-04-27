#include <random>
#include <time.h>

#include "bnRollHeal.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnPlayer.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

#include "bnChipSummonHandler.h"
#include "bnRollHeart.h"

#define RESOURCE_PATH "resources/spells/spell_roll.animation"

RollHeal::RollHeal(ChipSummonHandler* _summons, int _heal) : Spell()
{
  summons = _summons;
  SetPassthrough(true);
  EnableTileHighlight(false); // Do not highlight where we move

  field = summons->GetCaller()->GetField();
  team = summons->GetCaller()->GetTeam();

  direction = Direction::NONE;
  deleted = false;
  progress = 0.0f;
  hitHeight = 0.0f;
  srand((unsigned int)time(nullptr));
  random = rand() % 20 - 20;

  heal = _heal;

  int lr = (team == Team::RED) ? 1 : -1;
  setScale(2.0f*lr, 2.0f);

  Battle::Tile* _tile = summons->GetCaller()->GetTile();

  this->field->AddEntity(*this, _tile->GetX(), _tile->GetY());

  AUDIO.Play(AudioType::APPEAR);

  setTexture(*TEXTURES.LoadTextureFromFile("resources/spells/spell_roll.png"), true);
  animationComponent.Setup(RESOURCE_PATH);
  animationComponent.Reload();
  
  /**
   * This is very convoluted and will change with the chip summon refactored
   * Essentially we nest callbacks
   * 
   * First Roll is IDLE. when the animation ends, we set the animation to MOVE
   * 
   * While roll is moving, we find the first enemy in the field.
   * We set our target named `attack`
   * 
   * After MOVE is over, we set the animation to ATTACKING
   * 
   * If we found a target, we add 3 callbacks to frames 4, 12, and 20 
   * to deal damage to the enemy
   * 
   * At the animation end, we set the final animation to MOVE
   * 
   * At the end of the last MOVE animation, we spawn a heart
   * and request the summon system to remove this entity
   */
  animationComponent.SetAnimation("ROLL_IDLE", [this] { 
    this->animationComponent.SetAnimation("ROLL_MOVE", [this] {

      bool found = false;

      Battle::Tile* next = nullptr;
      Battle::Tile* attack = nullptr;
      while(field->GetNextTile(next)) {
        if (!found) {
          if (next->ContainsEntityType<Character>() && next->GetTeam() != this->GetTeam()) {
            this->GetTile()->RemoveEntityByID(this->GetID());

            Battle::Tile* prev = field->GetAt(next->GetX() - 1, next->GetY());
            prev->AddEntity(*this);

            attack = next;

            found = true;
          }
        }
      }

      if (found) {
        this->animationComponent.SetAnimation("ROLL_ATTACKING", [this] {
          this->animationComponent.SetAnimation("ROLL_MOVE", [this] {
            this->summons->SummonEntity(new RollHeart(this->summons, this->heal));
            this->summons->RemoveEntity(this);
          });
        });

        if (attack) {
          this->animationComponent.AddCallback(4,  [this, attack]() { attack->AffectEntities(this); }, std::function<void()>(), true);
          this->animationComponent.AddCallback(12, [this, attack]() { attack->AffectEntities(this); }, std::function<void()>(), true);
          this->animationComponent.AddCallback(20, [this, attack]() { attack->AffectEntities(this); }, std::function<void()>(), true);
        }
      }
      else {
        this->animationComponent.SetAnimation("ROLL_MOVE", [this] {
          this->summons->SummonEntity(new RollHeart(this->summons, this->heal));
          this->summons->RemoveEntity(this);
        });
      }
    });
  });
}

RollHeal::~RollHeal() {
}

void RollHeal::Update(float _elapsed) {
  animationComponent.Update(_elapsed);

  if (tile != nullptr) {
    setPosition(tile->getPosition());
  }

  Entity::Update(_elapsed);
}

bool RollHeal::Move(Direction _direction) {
  return false;
}

void RollHeal::Attack(Character* _entity) {
  if (_entity && _entity->GetTeam() != this->GetTeam()) {
    if (!_entity->IsPassthrough()) {
      auto props = Hit::DefaultProperties;
      props.damage = heal;
      _entity->Hit(props);

      int i = 1;

      if (rand() % 2 == 0) i = -1;

      if (_entity) {
        _entity->setPosition(_entity->getPosition().x + (i*(rand() % 4)), _entity->getPosition().y + (i*(rand() % 4)));

        hitHeight = _entity->GetHitHeight();
      }

      AUDIO.Play(AudioType::HURT);
    }
  }
}
