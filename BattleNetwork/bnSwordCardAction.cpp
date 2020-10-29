#include "bnSwordCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnBasicSword.h"
#include "bnSwordEffect.h"

#define PATH "resources/spells/spell_sword_blades.png"
#define ANIM "resources/spells/spell_sword_blades.animation"

#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.5 }

#define FRAMES FRAME1, FRAME2, FRAME3

SwordCardAction::SwordCardAction(Character * owner, int damage) : 
  CardAction(*owner, "PLAYER_SWORD")
  {
  SwordCardAction::damage = damage;

  blade = new SpriteProxyNode();
  blade->setTexture(TextureResourceManager::GetInstance().LoadTextureFromFile(PATH));
  blade->SetLayer(-2);

  hilt = new SpriteProxyNode();
  hilt->setTexture(owner->getTexture());
  hilt->SetLayer(-1);
  hilt->EnableParentShader(true);

  bladeAnim = Animation(ANIM);
  bladeAnim.SetAnimation("DEFAULT");

  auto userAnim = owner->GetFirstComponent<AnimationComponent>();
  hiltAnim = Animation(userAnim->GetFilePath());
  hiltAnim.Reload();
  hiltAnim.SetAnimation("HILT");

  auto& hiltAttachment = AddAttachment(userAnim->GetAnimationObj(), "HILT", *hilt).UseAnimation(hiltAnim);
  hiltAttachment.AddAttachment(hiltAnim, "ENDPOINT", *blade).UseAnimation(bladeAnim);

  element = Element::none;

  OverrideAnimationFrames({ FRAMES });
}

SwordCardAction::~SwordCardAction()
{
  delete blade, hilt;
}

void SwordCardAction::Execute() {
  // On attack frame, drop sword hitbox
  auto onTrigger = [this]() -> void {
    OnSpawnHitbox();
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
  auto field = GetOwner()->GetField();

  SwordEffect* e = new SwordEffect(field);
  field->AddEntity(*e, GetOwner()->GetTile()->GetX() + 1, GetOwner()->GetTile()->GetY());

  BasicSword* b = new BasicSword(field, GetOwner()->GetTeam(), damage);
  auto props = b->GetHitboxProperties();
  props.aggressor = GetOwnerAs<Character>();

  b->SetHitboxProperties(props);

  AUDIO.Play(AudioType::SWORD_SWING);

  field->AddEntity(*b, GetOwner()->GetTile()->GetX() + 1, GetOwner()->GetTile()->GetY());
}

void SwordCardAction::SetElement(Element elem)
{
  element = elem;
}

const Element SwordCardAction::GetElement() const
{
  return element;
}

void SwordCardAction::OnUpdate(float _elapsed)
{
  CardAction::OnUpdate(_elapsed);
}

void SwordCardAction::OnAnimationEnd()
{
}

void SwordCardAction::EndAction()
{
  Eject();
}