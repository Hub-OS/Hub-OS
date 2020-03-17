#include "bnThunderCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnThunder.h"

ThunderCardAction::ThunderCardAction(Character * owner, int damage) : CardAction(owner, "PLAYER_SHOOTING", &attachment, "BUSTER"),
attachmentAnim(owner->GetFirstComponent<AnimationComponent>()->GetFilePath()) {
  this->damage = damage;

  this->attachment = new SpriteProxyNode();
  this->attachment->setTexture(owner->getTexture());
  this->attachment->SetLayer(-1);

  attachmentAnim = Animation(owner->GetFirstComponent<AnimationComponent>()->GetFilePath());
  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("BUSTER");
}

void ThunderCardAction::Execute() {
  auto owner = GetOwner();

  owner->AddNode(this->attachment);
  this->attachment->EnableParentShader();
  attachmentAnim.Update(0, *this->attachment);

  // On shoot frame, drop projectile
  auto onFire = [this]() -> void {
    auto* thunder = new Thunder(GetOwner()->GetField(), GetOwner()->GetTeam());
    auto props = thunder->GetHitboxProperties();
    props.damage = this->damage;
    props.aggressor = GetOwner();
    thunder->SetHitboxProperties(props);
    GetOwner()->GetField()->AddEntity(*thunder, GetOwner()->GetTile()->GetX() + 1, GetOwner()->GetTile()->GetY());
  };

  this->AddAction(1, onFire);
}

ThunderCardAction::~ThunderCardAction()
{
  if (attachment) {
    delete attachment;
  }
}

void ThunderCardAction::OnUpdate(float _elapsed)
{
  attachmentAnim.Update(_elapsed, *this->attachment);
  CardAction::OnUpdate(_elapsed);
}

void ThunderCardAction::EndAction()
{
  this->GetOwner()->RemoveNode(attachment);
  GetOwner()->FreeComponentByID(this->GetID());

  delete this;
}
