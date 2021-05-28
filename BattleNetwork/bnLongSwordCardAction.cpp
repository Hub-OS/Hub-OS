#include "bnLongSwordCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnBasicSword.h"
#include "bnSwordEffect.h"

LongSwordCardAction::LongSwordCardAction(Character& actor, int damage) : 
  SwordCardAction(actor, damage) {
  LongSwordCardAction::damage = damage;
}

LongSwordCardAction::~LongSwordCardAction()
{
}

void LongSwordCardAction::OnSpawnHitbox(Entity::ID_t userId)
{
  Audio().Play(AudioType::SWORD_SWING);
  auto owner = &GetActor();
  auto field = owner->GetField();

  auto tiles = std::vector{
    owner->GetTile()->Offset(1, 0),
    owner->GetTile()->Offset(2, 0)
  };
 
  // this is the sword visual effect

  if (tiles[0]) {
    SwordEffect* e = new SwordEffect;
    e->SetAnimation("LONG");
    
    if (owner->GetFacing() == Direction::right) {
      e->setScale(-2.f, 2.f);
    }
    
    field->AddEntity(*e, *tiles[0]);
  }

  // Basic sword properties & hitbox
  if (tiles[0]) {
    BasicSword* b = new BasicSword(owner->GetTeam(), damage);
    auto props = b->GetHitboxProperties();
    props.element = GetElement();
    props.aggressor = userId;
    b->SetHitboxProperties(props);
    field->AddEntity(*b, *tiles[0]);
  }

  if (tiles[1]) {
    // resuse props with new hitbox
    BasicSword* b = new BasicSword(owner->GetTeam(), damage);
    auto props = b->GetHitboxProperties();
    b->SetHitboxProperties(props);

    field->AddEntity(*b, *tiles[1]);
  }
}