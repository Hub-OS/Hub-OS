#include "bnSwordCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnBasicSword.h"
#include "bnSwordEffect.h"

#define PATH "resources/spells/spell_sword_blades.png"
#define ANIM "resources/spells/spell_sword_blades.animation"

#define FRAME1 { 1, 0.1666 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.05 }
#define FRAME4 { 4, 0.5 }

#define FRAMES FRAME1, FRAME2, FRAME3, FRAME4

SwordCardAction::SwordCardAction(Character& user, int damage) : 
CardAction(user, "PLAYER_SWORD") {
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

  auto userAnim = GetCharacter().GetFirstComponent<AnimationComponent>();
  hiltAnim = Animation(userAnim->GetFilePath());
  hiltAnim.Reload();
  hiltAnim.SetAnimation("HILT");

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

    auto userAnim = GetCharacter().GetFirstComponent<AnimationComponent>();

    auto& hiltAttachment = AddAttachment(userAnim->GetAnimationObject(), "HILT", *hilt).UseAnimation(hiltAnim);
    hiltAttachment.AddAttachment(hiltAnim, "ENDPOINT", *blade).UseAnimation(bladeAnim);
  };

  switch (GetElement()) {
    case Element::fire:
      blade->setColor(sf::Color::Red);
      break;
    case Element::aqua:
      blade->setColor(sf::Color::Green);
      break;
  }

  AddAnimAction(2, onTrigger);
}

void SwordCardAction::OnSpawnHitbox()
{
  auto field = GetCharacter().GetField();
  auto tile = GetCharacter().GetTile()->Offset(1, 0);

  if (tile) {
    // visual fx
    SwordEffect* e = new SwordEffect;
    field->AddEntity(*e, *tile);

    BasicSword* b = new BasicSword(GetCharacter().GetTeam(), damage);
    auto props = b->GetHitboxProperties();
    props.aggressor = &GetCharacter();
    b->SetHitboxProperties(props);

    // add the hitbox beneath the visual fx
    field->AddEntity(*b, *tile);
  }

  Audio().Play(AudioType::SWORD_SWING);
}

void SwordCardAction::SetElement(Element elem)
{
  element = elem;
}

const Element SwordCardAction::GetElement() const
{
  return element;
}

void SwordCardAction::OnAnimationEnd()
{
}

void SwordCardAction::OnEndAction()
{
}