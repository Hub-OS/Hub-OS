#include "bnElecSwordCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

#define PATH "resources/spells/spell_elec_sword.png"
#define ANIM "resources/spells/spell_elec_sword.animation"

#define FRAME1 { 1, 0.1666 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.05 }
#define FRAME4 { 4, 0.5 }

#define FRAMES FRAME1, FRAME2, FRAME3, FRAME4

ElecSwordCardAction::ElecSwordCardAction(Character* actor, int damage) : 
  LongSwordCardAction(actor, damage) {
  ElecSwordCardAction::damage = damage;
  this->SetElement(Element::elec);
  blade->setTexture(Textures().LoadTextureFromFile(PATH));
  blade->setTexture(Textures().LoadTextureFromFile(PATH));
  bladeAnim = Animation(ANIM);
  bladeAnim.Reload();
  bladeAnim.SetAnimation("DEFAULT");

  OverrideAnimationFrames({ FRAMES });
}

ElecSwordCardAction::~ElecSwordCardAction()
{
}