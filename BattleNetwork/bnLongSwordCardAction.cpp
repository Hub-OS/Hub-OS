#include "bnLongSwordCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnBasicSword.h"
#include "bnSwordEffect.h"

LongSwordCardAction::LongSwordCardAction(Character& owner, int damage) : 
  SwordCardAction(owner, damage) {
  LongSwordCardAction::damage = damage;
}

LongSwordCardAction::~LongSwordCardAction()
{
}

void LongSwordCardAction::OnSpawnHitbox()
{
  Audio().Play(AudioType::SWORD_SWING);
  auto owner = &GetCharacter();
  auto field = GetCharacter().GetField();

  auto tiles = std::vector{
    owner->GetTile()->Offset(1, 0),
    owner->GetTile()->Offset(2, 0)
  };
 
  // this is the sword visual effect

  SwordEffect* e = new SwordEffect;
  e->SetAnimation("LONG");
  field->AddEntity(*e, *tiles[0]);

  // Basic sword properties & hitbox
  BasicSword* b = new BasicSword(owner->GetTeam(), damage);
  auto props = b->GetHitboxProperties();
  props.element = GetElement();
  props.aggressor = owner;
  b->SetHitboxProperties(props);
  field->AddEntity(*b, *tiles[0]);

  // resuse props with new hitbox
  b = new BasicSword(owner->GetTeam(), damage);
  b->SetHitboxProperties(props);

  field->AddEntity(*b, *tiles[1]);
}