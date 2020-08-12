#include "bnWideSwordCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnBasicSword.h"
#include "bnSwordEffect.h"

WideSwordCardAction::WideSwordCardAction(Character * owner, int damage) : SwordCardAction(owner, damage) {
  WideSwordCardAction::damage = damage;
}

WideSwordCardAction::~WideSwordCardAction()
{
}

void WideSwordCardAction::OnSpawnHitbox()
{
  auto field = GetOwner()->GetField();

  SwordEffect* e = new SwordEffect(field);
  e->SetAnimation("WIDE");
  field->AddEntity(*e, GetOwner()->GetTile()->GetX() + 1, GetOwner()->GetTile()->GetY());

  BasicSword* b = new BasicSword(field, GetOwner()->GetTeam(), damage);
  auto props = b->GetHitboxProperties();
  props.aggressor = GetOwnerAs<Character>();
  b->SetHitboxProperties(props);

  AUDIO.Play(AudioType::SWORD_SWING);
  field ->AddEntity(*b, GetOwner()->GetTile()->GetX() + 1, GetOwner()->GetTile()->GetY());

  b = new BasicSword(field, GetOwner()->GetTeam(), damage);
  b->SetHitboxProperties(props);
  field->AddEntity(*b, GetOwner()->GetTile()->GetX() + 1, GetOwner()->GetTile()->GetY() + 1);

  b = new BasicSword(field, GetOwner()->GetTeam(), damage);
  b->SetHitboxProperties(props);
  field->AddEntity(*b, GetOwner()->GetTile()->GetX() + 1, GetOwner()->GetTile()->GetY() - 1);
}