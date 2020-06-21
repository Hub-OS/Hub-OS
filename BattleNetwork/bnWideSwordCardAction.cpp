#include "bnWideSwordCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnBasicSword.h"

WideSwordCardAction::WideSwordCardAction(Character * owner, int damage) : SwordCardAction(owner, damage) {
  WideSwordCardAction::damage = damage;
}

WideSwordCardAction::~WideSwordCardAction()
{
}

void WideSwordCardAction::OnSpawnHitbox()
{
  BasicSword* b = new BasicSword(GetOwner()->GetField(), GetOwner()->GetTeam(), damage);
  auto props = b->GetHitboxProperties();
  props.aggressor = GetOwnerAs<Character>();
  b->SetHitboxProperties(props);

  AUDIO.Play(AudioType::SWORD_SWING);
  GetOwner()->GetField()->AddEntity(*b, GetOwner()->GetTile()->GetX() + 1, GetOwner()->GetTile()->GetY());

  b = new BasicSword(GetOwner()->GetField(), GetOwner()->GetTeam(), damage);
  b->SetHitboxProperties(props);
  GetOwner()->GetField()->AddEntity(*b, GetOwner()->GetTile()->GetX() + 1, GetOwner()->GetTile()->GetY() + 1);

  b = new BasicSword(GetOwner()->GetField(), GetOwner()->GetTeam(), damage);
  b->SetHitboxProperties(props);
  GetOwner()->GetField()->AddEntity(*b, GetOwner()->GetTile()->GetX() + 1, GetOwner()->GetTile()->GetY() - 1);
}