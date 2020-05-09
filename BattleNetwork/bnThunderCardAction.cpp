#include "bnThunderCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnThunder.h"

ThunderCardAction::ThunderCardAction(Character * user, int damage) : CardAction(user, "PLAYER_SHOOTING", &attachment, "BUSTER"),
attachmentAnim(user->GetFirstComponent<AnimationComponent>()->GetFilePath()) {
  ThunderCardAction::damage = damage;

  attachment = new SpriteProxyNode();
  attachment->setTexture(user->getTexture());
  attachment->SetLayer(-1);

  attachmentAnim = Animation(user->GetFirstComponent<AnimationComponent>()->GetFilePath());
  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("BUSTER");
}

void ThunderCardAction::Execute() {
  auto user = GetUser();

  user->AddNode(attachment);
  attachment->EnableParentShader();
  attachmentAnim.Update(0, attachment->getSprite());

  // On shoot frame, drop projectile
  auto onFire = [this, user]() -> void {
    auto* thunder = new Thunder(user->GetField(), user->GetTeam());
    auto props = thunder->GetHitboxProperties();
    props.damage = damage;
    props.aggressor = user;
    thunder->SetHitboxProperties(props);
    user->GetField()->AddEntity(*thunder, user->GetTile()->GetX() + 1, user->GetTile()->GetY());
  };

  AddAction(1, onFire);
}

ThunderCardAction::~ThunderCardAction()
{
  if (attachment) {
    delete attachment;
  }
}

void ThunderCardAction::OnUpdate(float _elapsed)
{
  attachmentAnim.Update(_elapsed, attachment->getSprite());
  CardAction::OnUpdate(_elapsed);
}

void ThunderCardAction::EndAction()
{
  GetUser()->RemoveNode(attachment);
  GetUser()->EndCurrentAction();
}
