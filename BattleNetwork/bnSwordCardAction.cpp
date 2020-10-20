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

SwordCardAction::SwordCardAction(Character * owner, int damage) : CardAction(owner, "PLAYER_SWORD", &attachment, "HILT"), attachmentAnim(ANIM) {
  SwordCardAction::damage = damage;

  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("DEFAULT");

  overlay.setTexture(*TextureResourceManager::GetInstance().LoadTextureFromFile(PATH));
  attachment = new SpriteProxyNode(overlay);
  attachment->SetLayer(-2);

  hiltAttachment = new SpriteProxyNode();
  hiltAttachment->setTexture(owner->getTexture());
  hiltAttachment->SetLayer(-1);

  hiltAttachmentAnim = Animation(owner->GetFirstComponent<AnimationComponent>()->GetFilePath());
  hiltAttachmentAnim.Reload();
  hiltAttachmentAnim.SetAnimation("HILT");

  overlay.setTexture(*TextureResourceManager::GetInstance().LoadTextureFromFile(PATH));
  attachmentAnim = Animation(ANIM);
  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("DEFAULT");

  element = Element::none;

  OverrideAnimationFrames({ FRAMES });
}

SwordCardAction::~SwordCardAction()
{
  if (attachment) {
    delete attachment;
  }

  if (hiltAttachment) {
    delete hiltAttachment;
  }
}

void SwordCardAction::Execute() {
  auto owner = GetOwner();
  owner->AddNode(hiltAttachment);
  hiltAttachmentAnim.Update(0, hiltAttachment->getSprite());
  hiltAttachment->EnableParentShader(true);

  hiltAttachment->AddNode(attachment);
  attachmentAnim.Update(0, attachment->getSprite());

  // On attack frame, drop sword hitbox
  auto onTrigger = [this, owner]() -> void {
    OnSpawnHitbox();
  };

  switch (GetElement()) {
    case Element::fire:
      attachment->setColor(sf::Color::Red);
      break;
    case Element::aqua:
      attachment->setColor(sf::Color::Green);
      break;
  }

  AddAction(2, onTrigger);

  OnUpdate(0); // position to owner...
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
  hiltAttachmentAnim.Update(_elapsed, hiltAttachment->getSprite());
  attachmentAnim.Update(_elapsed, attachment->getSprite());

  // update node position in the animation:
  // Position the hilt
  auto baseOffset = anim->GetPoint("HILT");
  auto origin = GetOwner()->getSprite().getOrigin();
  baseOffset = baseOffset - origin;
  hiltAttachment->setPosition(baseOffset);

  // position the blade
  baseOffset = hiltAttachmentAnim.GetPoint("endpoint");
  origin = hiltAttachment->getOrigin();
  baseOffset = baseOffset - origin;
  attachment->setPosition(baseOffset);
}

void SwordCardAction::OnAnimationEnd()
{
}

void SwordCardAction::EndAction()
{
  GetOwner()->RemoveNode(hiltAttachment);
  hiltAttachment->RemoveNode(attachment);
  Eject();
}