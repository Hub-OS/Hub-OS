#include "bnWideSwordChipAction.h"
#include "bnChipAction.h"
#include "bnSpriteSceneNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnBasicSword.h"

WideSwordChipAction::WideSwordChipAction(Character * owner, int damage) : SwordChipAction(owner, damage) {
  this->damage = damage;
}

WideSwordChipAction::~WideSwordChipAction()
{
}

void WideSwordChipAction::OnSpawnHitbox()
{
  BasicSword* b = new BasicSword(GetOwner()->GetField(), GetOwner()->GetTeam(), damage);
  auto props = b->GetHitboxProperties();
  props.aggressor = GetOwnerAs<Character>();
  b->SetHitboxProperties(props);

  AUDIO.Play(AudioType::SWORD_SWING);

  GetOwner()->GetField()->AddEntity(*b, GetOwner()->GetTile()->GetX() + 1, GetOwner()->GetTile()->GetY());

  b = new BasicSword(GetOwner()->GetField(), GetOwner()->GetTeam(), damage);
  // resuse props
  b->SetHitboxProperties(props);

  GetOwner()->GetField()->AddEntity(*b, GetOwner()->GetTile()->GetX() + 1, GetOwner()->GetTile()->GetY() + 1);
}