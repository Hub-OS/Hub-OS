#include "bnWideSwordCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnBasicSword.h"

WideSwordCardAction::WideSwordCardAction(Character * user, int damage) : SwordCardAction(user, damage) {
  WideSwordCardAction::damage = damage;
}

WideSwordCardAction::~WideSwordCardAction()
{
}

void WideSwordCardAction::OnSpawnHitbox()
{
  BasicSword* b = new BasicSword(GetUser()->GetField(), GetUser()->GetTeam(), damage);
  auto props = b->GetHitboxProperties();
  props.aggressor = GetUser();
  b->SetHitboxProperties(props);

  AUDIO.Play(AudioType::SWORD_SWING);

  GetUser()->GetField()->AddEntity(*b, GetUser()->GetTile()->GetX() + 1, GetUser()->GetTile()->GetY());

  b = new BasicSword(GetUser()->GetField(), GetUser()->GetTeam(), damage);
  // resuse props
  b->SetHitboxProperties(props);

  GetUser()->GetField()->AddEntity(*b, GetUser()->GetTile()->GetX() + 1, GetUser()->GetTile()->GetY() + 1);
}