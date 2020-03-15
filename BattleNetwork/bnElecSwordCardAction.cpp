#include "bnElecSwordCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

#define PATH "resources/spells/spell_elec_sword.png"
#define ANIM "resources/spells/spell_elec_sword.animation"

#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.3 }

#define FRAMES FRAME1, FRAME2, FRAME3

ElecSwordCardAction::ElecSwordCardAction(Character * owner, int damage) : LongSwordCardAction(owner, damage) {
  this->damage = damage;

  overlay.setTexture(*TextureResourceManager::GetInstance().LoadTextureFromFile(PATH));
  attachmentAnim = Animation(ANIM);
  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("DEFAULT");

  this->OverrideAnimationFrames({ FRAMES });
}

ElecSwordCardAction::~ElecSwordCardAction()
{
}