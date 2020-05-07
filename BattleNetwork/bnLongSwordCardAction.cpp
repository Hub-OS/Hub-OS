#include "bnLongSwordCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnBasicSword.h"

LongSwordCardAction::LongSwordCardAction(Character * owner, int damage) : SwordCardAction(owner, damage) {
  LongSwordCardAction::damage = damage;
}

LongSwordCardAction::~LongSwordCardAction()
{
}

void LongSwordCardAction::OnSpawnHitbox()
{
  BasicSword* b = new BasicSword(GetOwner()->GetField(), GetOwner()->GetTeam(), damage);
  auto props = b->GetHitboxProperties();
  props.element = GetElement();
  props.aggressor = GetOwnerAs<Character>();
  b->SetHitboxProperties(props);

  AUDIO.Play(AudioType::SWORD_SWING);

  GetOwner()->GetField()->AddEntity(*b, GetOwner()->GetTile()->GetX() + 1, GetOwner()->GetTile()->GetY());

  b = new BasicSword(GetOwner()->GetField(), GetOwner()->GetTeam(), damage);
  // resuse props
  b->SetHitboxProperties(props);

  GetOwner()->GetField()->AddEntity(*b, GetOwner()->GetTile()->GetX() + 2, GetOwner()->GetTile()->GetY());
}