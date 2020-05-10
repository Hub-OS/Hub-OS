#include "bnThunderCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnThunder.h"

ThunderCardAction::ThunderCardAction(Character& user, int damage) : CardAction(user, "PLAYER_SHOOTING") {
  ThunderCardAction::damage = damage;

  attachment = new SpriteProxyNode();
  attachment->setTexture(user.getTexture());
  attachment->SetLayer(-1);
  attachment->EnableParentShader();

  attachmentAnim = Animation(user.GetFirstComponent<AnimationComponent>()->GetFilePath());
  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("BUSTER");

  Animation& userAnim = user.GetFirstComponent<AnimationComponent>()->GetAnimationObject();
  AddAttachment(userAnim, "BUSTER", *attachment).PrepareAnimation(attachmentAnim);
}

void ThunderCardAction::OnExecute() {
  // On shoot frame, drop projectile
  auto onFire = [this]() -> void {
    auto& user = GetUser();

    auto* thunder = new Thunder(user.GetField(), user.GetTeam());
    auto props = thunder->GetHitboxProperties();
    props.damage = damage;
    props.aggressor = &user;
    thunder->SetHitboxProperties(props);
    user.GetField()->AddEntity(*thunder, user.GetTile()->GetX() + 1, user.GetTile()->GetY());
  };

  AddAction(1, onFire);
}

ThunderCardAction::~ThunderCardAction()
{
  if (attachment) {
    delete attachment;
  }
}

void ThunderCardAction::OnEndAction()
{
  GetUser().RemoveNode(attachment);
}
