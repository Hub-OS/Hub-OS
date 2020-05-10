#include "bnWideSwordCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnBasicSword.h"

WideSwordCardAction::WideSwordCardAction(Character& user, int damage) : SwordCardAction(user, damage) {
  WideSwordCardAction::damage = damage;
}

WideSwordCardAction::~WideSwordCardAction()
{
}

void WideSwordCardAction::OnSpawnHitbox()
{
  auto& user = GetUser();
  BasicSword* b = new BasicSword(user.GetField(), user.GetTeam(), damage);
  auto props = b->GetHitboxProperties();
  props.aggressor = &user;
  b->SetHitboxProperties(props);

  AUDIO.Play(AudioType::SWORD_SWING);

  user.GetField()->AddEntity(*b, user.GetTile()->GetX() + 1, user.GetTile()->GetY());

  b = new BasicSword(user.GetField(), user.GetTeam(), damage);
  // resuse props
  b->SetHitboxProperties(props);

  GetUser().GetField()->AddEntity(*b, user.GetTile()->GetX() + 1, user.GetTile()->GetY() + 1);
}