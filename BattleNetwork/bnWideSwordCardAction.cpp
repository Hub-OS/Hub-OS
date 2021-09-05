#include "bnWideSwordCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnBasicSword.h"
#include "bnSwordEffect.h"
#include "bnCharacter.h"

WideSwordCardAction::WideSwordCardAction(std::shared_ptr<Character> actor, int damage) : SwordCardAction(actor, damage) {
  WideSwordCardAction::damage = damage;
}

WideSwordCardAction::~WideSwordCardAction()
{
}

void WideSwordCardAction::OnSpawnHitbox(Entity::ID_t userId)
{
  auto actor = this->GetActor();
  auto field = actor->GetField();

  int step = actor->GetFacing() == Direction::left ? -1 : 1;

  auto tiles = std::vector{
    actor->GetTile()->Offset(step, 0),
    actor->GetTile()->Offset(step, 1),
    actor->GetTile()->Offset(step,-1)
  };

  auto e = std::make_shared<SwordEffect>();

  if (step == -1) {
    e->setScale(-2.f, 2.f);
  }

  e->SetAnimation("WIDE");
  field->AddEntity(e, *tiles[0]);

  auto b = std::make_shared<BasicSword>(actor->GetTeam(), damage);
  auto props = b->GetHitboxProperties();
  props.aggressor = userId;
  b->SetHitboxProperties(props);

  Audio().Play(AudioType::SWORD_SWING);
  field ->AddEntity(b, *tiles[0]);

  b = std::make_shared<BasicSword>(actor->GetTeam(), damage);
  b->SetHitboxProperties(props);
  field->AddEntity(b, *tiles[1]);
 
  b = std::make_shared<BasicSword>(actor->GetTeam(), damage);
  b->SetHitboxProperties(props);
  field->AddEntity(b, *tiles[2]);
}