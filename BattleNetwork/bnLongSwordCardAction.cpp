#include "bnLongSwordCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnBasicSword.h"
#include "bnSwordEffect.h"

LongSwordCardAction::LongSwordCardAction(Character * owner, int damage) : 
  SwordCardAction(owner, damage) {
  LongSwordCardAction::damage = damage;
}

LongSwordCardAction::~LongSwordCardAction()
{
}

void LongSwordCardAction::OnSpawnHitbox()
{
  Audio().Play(AudioType::SWORD_SWING);
  auto field = GetOwner()->GetField();

  SwordEffect* e = new SwordEffect(field);
  e->SetAnimation("LONG");

  field->AddEntity(*e, GetOwner()->GetTile()->GetX() + 1, GetOwner()->GetTile()->GetY());

  BasicSword* b = new BasicSword(field, GetOwner()->GetTeam(), damage);

  auto props = b->GetHitboxProperties();
  props.element = GetElement();
  props.aggressor = &user;
  b->SetHitboxProperties(props);

  field->AddEntity(*b, GetOwner()->GetTile()->GetX() + 1, GetOwner()->GetTile()->GetY());

  b = new BasicSword(field, GetOwner()->GetTeam(), damage);
  // resuse props
  b->SetHitboxProperties(props);

  field->AddEntity(*b, GetOwner()->GetTile()->GetX() + 2, GetOwner()->GetTile()->GetY());
}