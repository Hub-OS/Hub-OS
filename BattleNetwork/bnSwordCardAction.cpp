#include "bnSwordCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteSceneNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnBasicSword.h"

#define PATH "resources/spells/spell_sword_blades.png"
#define ANIM "resources/spells/spell_sword_blades.animation"

#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.3 }

#define FRAMES FRAME1, FRAME2, FRAME3

SwordCardAction::SwordCardAction(Character * owner, int damage) : CardAction(owner, "PLAYER_SWORD", &attachment, "HILT"), attachmentAnim(ANIM) {
  this->damage = damage;

  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("DEFAULT");

  overlay.setTexture(*TextureResourceManager::GetInstance().LoadTextureFromFile(PATH));
  this->attachment = new SpriteSceneNode(overlay);
  this->attachment->SetLayer(-2);

  this->hiltAttachment = new SpriteSceneNode();
  this->hiltAttachment->setTexture(*owner->getTexture());
  this->hiltAttachment->SetLayer(-1);

  hiltAttachmentAnim = Animation(owner->GetFirstComponent<AnimationComponent>()->GetFilePath());
  hiltAttachmentAnim.Reload();
  hiltAttachmentAnim.SetAnimation("HILT");

  overlay.setTexture(*TextureResourceManager::GetInstance().LoadTextureFromFile(PATH));
  attachmentAnim = Animation(ANIM);
  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("DEFAULT");

  element = Element::NONE;

  this->OverrideAnimationFrames({ FRAMES });
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
  owner->AddNode(this->hiltAttachment);
  hiltAttachmentAnim.Update(0, *this->hiltAttachment);
  this->hiltAttachment->EnableParentShader(true);

  hiltAttachment->AddNode(attachment);
  attachmentAnim.Update(0, *this->attachment);

  // On attack frame, drop sword hitbox
  auto onTrigger = [this, owner]() -> void {
    this->OnSpawnHitbox();
  };

  switch (GetElement()) {
    case Element::FIRE:
      attachment->setColor(sf::Color::Red);
      break;
    case Element::AQUA:
      attachment->setColor(sf::Color::Green);
      break;
  }

  this->AddAction(2, onTrigger);

  this->OnUpdate(0); // position to owner...
}

void SwordCardAction::OnSpawnHitbox()
{
  BasicSword* b = new BasicSword(GetOwner()->GetField(), GetOwner()->GetTeam(), damage);
  auto props = b->GetHitboxProperties();
  props.aggressor = GetOwnerAs<Character>();

  b->SetHitboxProperties(props);

  AUDIO.Play(AudioType::SWORD_SWING);

  GetOwner()->GetField()->AddEntity(*b, GetOwner()->GetTile()->GetX() + 1, GetOwner()->GetTile()->GetY());
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
  hiltAttachmentAnim.Update(_elapsed, *this->hiltAttachment);
  attachmentAnim.Update(_elapsed, *this->attachment);

  // update node position in the animation:
  // Position the hilt
  auto baseOffset = this->anim->GetPoint("HILT");
  auto origin = GetOwner()->getSprite().getOrigin();
  baseOffset = baseOffset - origin;
  hiltAttachment->setPosition(baseOffset);

  // position the blade
  baseOffset = hiltAttachmentAnim.GetPoint("endpoint");
  origin = hiltAttachment->getOrigin();
  baseOffset = baseOffset - origin;
  attachment->setPosition(baseOffset);
}

void SwordCardAction::EndAction()
{
  this->GetOwner()->RemoveNode(hiltAttachment);
  hiltAttachment->RemoveNode(attachment);
  GetOwner()->FreeComponentByID(this->GetID());

  delete this;
}