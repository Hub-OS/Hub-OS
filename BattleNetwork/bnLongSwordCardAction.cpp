#include "bnLongSwordCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnBasicSword.h"

LongSwordCardAction::LongSwordCardAction(Character& owner, int damage) : SwordCardAction(owner, damage) {
  LongSwordCardAction::damage = damage;
}

LongSwordCardAction::~LongSwordCardAction()
{
}

void LongSwordCardAction::OnSpawnHitbox()
{
  auto& user = GetUser();
  BasicSword* b = new BasicSword(user.GetField(), user.GetTeam(), damage);
  auto props = b->GetHitboxProperties();
  props.element = GetElement();
  props.aggressor = &user;
  b->SetHitboxProperties(props);

  Audio().Play(AudioType::SWORD_SWING);

  user.GetField()->AddEntity(*b, user.GetTile()->GetX() + 1, user.GetTile()->GetY());

  b = new BasicSword(user.GetField(), user.GetTeam(), damage);

  // resuse props
  b->SetHitboxProperties(props);

  user.GetField()->AddEntity(*b, user.GetTile()->GetX() + 2, user.GetTile()->GetY());
}