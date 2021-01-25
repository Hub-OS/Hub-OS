#include "bnThunderCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnThunder.h"

ThunderCardAction::ThunderCardAction(Character& owner, int damage) :
  CardAction(owner, "PLAYER_SHOOTING"),
  attachmentAnim(owner.GetFirstComponent<AnimationComponent>()->GetFilePath()) {
  ThunderCardAction::damage = damage;

  attachment = new SpriteProxyNode();
  attachment->setTexture(owner.getTexture());
  attachment->SetLayer(-1);
  attachment->EnableParentShader();

  attachmentAnim = Animation(owner.GetFirstComponent<AnimationComponent>()->GetFilePath());
  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("BUSTER");
  AddAttachment(owner, "buster", *attachment).UseAnimation(attachmentAnim);
}

void ThunderCardAction::OnExecute() {
  // On shoot frame, drop projectile
  auto onFire = [this]() -> void {

    Team team = GetOwner()->GetTeam();
    auto* thunder = new Thunder(team);
    auto props = thunder->GetHitboxProperties();
    props.damage = damage;
    props.aggressor = GetOwner();
    thunder->SetHitboxProperties(props);

    int step = 1;

    if (team != Team::red) {
      step = -1;
    }

    GetOwner()->GetField()->AddEntity(*thunder, GetOwner()->GetTile()->GetX() + step, GetOwner()->GetTile()->GetY());
  };

  AddAnimAction(1, onFire);
}

ThunderCardAction::~ThunderCardAction()
{
  if (attachment) {
    delete attachment;
  }
}

void ThunderCardAction::OnAnimationEnd()
{
}

void ThunderCardAction::OnEndAction()
{
  Eject();
}
