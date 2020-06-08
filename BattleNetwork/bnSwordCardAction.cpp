#include "bnSwordCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnBasicSword.h"

#define PATH "resources/spells/spell_sword_blades.png"
#define ANIM "resources/spells/spell_sword_blades.animation"

#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.3 }

#define FRAMES FRAME1, FRAME2, FRAME3

SwordCardAction::SwordCardAction(Character& user, int damage) : CardAction(user, "PLAYER_SWORD") {
  SwordCardAction::damage = damage;

  blade = new SpriteProxyNode();
  blade->setTexture(Textures().LoadTextureFromFile(PATH));
  blade->SetLayer(-2);

  hilt = new SpriteProxyNode();
  hilt->setTexture(user.getTexture());
  hilt->SetLayer(-1);
  hilt->EnableParentShader(true);

  bladeAnim = Animation(ANIM);
  bladeAnim.SetAnimation("DEFAULT");

  auto userAnim = user.GetFirstComponent<AnimationComponent>();
  hiltAnim = Animation(userAnim->GetFilePath());
  hiltAnim.Reload();
  hiltAnim.SetAnimation("HILT");

  AddAttachment(userAnim->GetAnimationObject(), "HILT", *hilt).PrepareAnimation(hiltAnim);
  AddAttachment(hiltAnim, "ENDPOINT", *blade).PrepareAnimation(bladeAnim);

  element = Element::none;

  OverrideAnimationFrames({ FRAMES });
}

SwordCardAction::~SwordCardAction()
{
  delete blade, hilt;
}

void SwordCardAction::OnExecute() {
  // On attack frame, drop sword hitbox
  auto onTrigger = [this]() -> void {
    OnSpawnHitbox();
  };

  AddAction(2, onTrigger);
}

void SwordCardAction::OnSpawnHitbox()
{
  auto& user = GetUser();
  BasicSword* b = new BasicSword(user.GetField(), user.GetTeam(), damage);
  auto props = b->GetHitboxProperties();
  props.aggressor = &user;

  b->SetHitboxProperties(props);

  Audio()().Play(AudioType::SWORD_SWING);

  user.GetField()->AddEntity(*b, user.GetTile()->GetX() + 1, user.GetTile()->GetY());
}

void SwordCardAction::SetElement(Element elem)
{
  element = elem;
}

const Element SwordCardAction::GetElement() const
{
  return element;
}

void SwordCardAction::OnEndAction()
{
  GetUser().RemoveNode(hilt);
  hilt->RemoveNode(blade);
}