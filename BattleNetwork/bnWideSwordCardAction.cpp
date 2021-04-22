#include "bnWideSwordCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnBasicSword.h"
#include "bnSwordEffect.h"

WideSwordCardAction::WideSwordCardAction(Character& actor, int damage) : SwordCardAction(actor, damage) {
  WideSwordCardAction::damage = damage;
}

WideSwordCardAction::~WideSwordCardAction()
{
}

void WideSwordCardAction::OnSpawnHitbox(Entity::ID_t userId)
{
  auto& actor = GetActor();
  auto field = actor.GetField();

  auto tiles = std::vector{
    actor.GetTile()->Offset(1, 0),
    actor.GetTile()->Offset(1, 1),
    actor.GetTile()->Offset(1,-1)
  };

  SwordEffect* e = new SwordEffect;
  e->SetAnimation("WIDE");
  field->AddEntity(*e, *tiles[0]);

  BasicSword* b = new BasicSword(actor.GetTeam(), damage);
  auto props = b->GetHitboxProperties();
  props.aggressor = userId;
  b->SetHitboxProperties(props);

  Audio().Play(AudioType::SWORD_SWING);
  field ->AddEntity(*b, *tiles[0]);

  b = new BasicSword(actor.GetTeam(), damage);
  b->SetHitboxProperties(props);
  field->AddEntity(*b, *tiles[1]);
  
  b = new BasicSword(actor.GetTeam(), damage);
  b->SetHitboxProperties(props);
  field->AddEntity(*b, *tiles[2]);
}