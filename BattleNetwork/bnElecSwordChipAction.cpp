#include "bnElecSwordChipAction.h"
#include "bnChipAction.h"
#include "bnSpriteSceneNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnBasicSword.h"

#define PATH "resources/spells/spell_elec_sword.png"
#define ANIM "resources/spells/spell_elec_sword.animation"

#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.3 }

#define FRAMES FRAME1, FRAME2, FRAME3

ElecSwordChipAction::ElecSwordChipAction(Character * owner, int damage) : SwordChipAction(owner, damage) {
  this->damage = damage;

  overlay.setTexture(*TextureResourceManager::GetInstance().LoadTextureFromFile(PATH));
  attachmentAnim = Animation(ANIM);
  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("DEFAULT");

  this->OverrideAnimationFrames({ FRAMES });
}

ElecSwordChipAction::~ElecSwordChipAction()
{
}

void ElecSwordChipAction::OnSpawnHitbox()
{
  BasicSword* b = new BasicSword(GetOwner()->GetField(), GetOwner()->GetTeam(), damage);
  auto props = b->GetHitboxProperties();
  props.aggressor = GetOwnerAs<Character>();
  props.element = Element::ELEC;
  props.flags |= Hit::stun;

  b->SetHitboxProperties(props);

  AUDIO.Play(AudioType::SWORD_SWING);

  GetOwner()->GetField()->AddEntity(*b, GetOwner()->GetTile()->GetX() + 1, GetOwner()->GetTile()->GetY());
}