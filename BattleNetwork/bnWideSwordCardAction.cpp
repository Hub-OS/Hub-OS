#include "bnWideSwordCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnBasicSword.h"
#include "bnSwordEffect.h"

WideSwordCardAction::WideSwordCardAction(Character& user, int damage) : SwordCardAction(user, damage) {
  WideSwordCardAction::damage = damage;
}

WideSwordCardAction::~WideSwordCardAction()
{
}

void WideSwordCardAction::OnSpawnHitbox()
{
  auto& owner = GetCharacter();
  auto field = GetCharacter().GetField();

  auto tiles = std::vector{
    owner.GetTile()->Offset(1, 0),
    owner.GetTile()->Offset(1, 1),
    owner.GetTile()->Offset(1,-1)
  };

  SwordEffect* e = new SwordEffect;
  e->SetAnimation("WIDE");
  field->AddEntity(*e, *tiles[0]);

  BasicSword* b = new BasicSword(owner.GetTeam(), damage);
  auto props = b->GetHitboxProperties();
  props.aggressor = owner.GetID();
  b->SetHitboxProperties(props);

  Audio().Play(AudioType::SWORD_SWING);
  field ->AddEntity(*b, *tiles[0]);

  b = new BasicSword(owner.GetTeam(), damage);
  b->SetHitboxProperties(props);
  field->AddEntity(*b, *tiles[1]);
  
  b = new BasicSword(owner.GetTeam(), damage);
  b->SetHitboxProperties(props);
  field->AddEntity(*b, *tiles[2]);
}