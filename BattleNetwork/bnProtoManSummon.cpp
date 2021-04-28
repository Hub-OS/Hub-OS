#include <random>
#include <time.h>
#include <algorithm>

#include "bnProtoManSummon.h"
#include "bnCharacter.h"
#include "bnObstacle.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnBasicSword.h"
#include "bnSwordEffect.h"

#define RESOURCE_PATH "resources/spells/protoman_summon.animation"

ProtoManSummon::ProtoManSummon(Character* user, int damage) : 
  user(user),
  Spell(user->GetTeam())
{
  SetPassthrough(true);
  random = rand() % 20 - 20;

  int lr = (team == Team::red) ? 1 : -1;
  setScale(2.0f*lr, 2.0f);

  Audio().Play(AudioType::APPEAR);

  setTexture(Textures().LoadTextureFromFile("resources/spells/protoman_summon.png"), true);

  animationComponent = CreateComponent<AnimationComponent>(this);
  animationComponent->SetPath(RESOURCE_PATH);
  animationComponent->Load();

  // TODO: noodely callbacks desgin might be best abstracted by ActionLists (use CardAction::Step) 2/27/2021
  animationComponent->SetAnimation("APPEAR", [this] {
  auto handleAttack = [this] () {
      DoAttackStep();
    };
    animationComponent->SetAnimation("MOVE", handleAttack);
  });

  auto props = GetHitboxProperties();
  props.damage = damage;
  props.flags |= Hit::flash;
  props.aggressor = user->GetID();
  SetHitboxProperties(props);
}

ProtoManSummon::~ProtoManSummon() {
}

void ProtoManSummon::DoAttackStep() {
  if (targets.size() > 0) {
    int step = -1;

    if (GetTeam() == Team::blue) {
      step = 1;
    }

    Battle::Tile* prev = field->GetAt(targets[0]->GetX() + step, targets[0]->GetY());
    GetTile()->RemoveEntityByID(GetID());
    prev->AddEntity(*this);

    animationComponent->SetAnimation("ATTACK", [this, prev] {
      animationComponent->SetAnimation("MOVE", [this, prev] {
      DoAttackStep();
      });
    });

    animationComponent->AddCallback(4,  [this]() {
      for (auto entity : targets[0]->FindEntities([](Entity* e) { return dynamic_cast<Character*>(e); })) {
        entity->GetTile()->AffectEntities(this);
      }

      targets.erase(targets.begin());
    }, true);
  }
  else {
    animationComponent->SetAnimation("MOVE", [this] {
      Delete();
      user->Reveal();
    });
  }
}

void ProtoManSummon::OnDelete()
{
  Remove();
}

void ProtoManSummon::OnSpawn(Battle::Tile& start)
{
  Battle::Tile* next = nullptr;

  auto allTiles = field->FindTiles([](Battle::Tile* tile) { return true; });
  auto iter = allTiles.begin();

  while (iter != allTiles.end()) {
    next = (*iter);

    if (next->ContainsEntityType<Character>() && !next->ContainsEntityType<Obstacle>() && next->GetTeam() != GetTeam()) {
      int step = -1;

      if (GetTeam() == Team::blue) {
        step = 1;
      }

      Battle::Tile* prev = field->GetAt(next->GetX() + step, next->GetY());

      auto characters = prev->FindEntities([this](Entity* in) {
        return this->user != in && (dynamic_cast<Character*>(in) && in->GetTeam() != Team::unknown);
        });

      bool blocked = (characters.size() > 0) || !prev->IsWalkable();

      if (!blocked) {
        targets.push_back(next);
      }
    }

    iter++;
  }
}

void ProtoManSummon::OnUpdate(double _elapsed) {
  if (tile != nullptr) {
    setPosition(tile->getPosition());
  }
}

void ProtoManSummon::Attack(Character* _entity) {
  auto field = GetField();

  auto tile = _entity->GetTile();

  SwordEffect* e = new SwordEffect;
  e->SetAnimation("WIDE");
  field->AddEntity(*e, tile->GetX(), tile->GetY());

  BasicSword* b = new BasicSword(GetTeam(), 0);
  auto props = this->GetHitboxProperties();
  props.aggressor = user->GetID();
  b->SetHitboxProperties(props);

  Audio().Play(AudioType::SWORD_SWING);
  field->AddEntity(*b, tile->GetX(),tile->GetY());

  b = new BasicSword(GetTeam(), 0);
  props = this->GetHitboxProperties();
  props.aggressor = user->GetID();
  b->SetHitboxProperties(props);
  field->AddEntity(*b,tile->GetX(), tile->GetY() + 1);

  b = new BasicSword(GetTeam(), 0);
  props = this->GetHitboxProperties();
  props.aggressor = user->GetID();
  field->AddEntity(*b, tile->GetX(), tile->GetY() - 1);

  Audio().Play(AudioType::SWORD_SWING);
}
