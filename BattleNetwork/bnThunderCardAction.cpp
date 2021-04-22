#include "bnThunderCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnThunder.h"
#include "bnField.h"

ThunderCardAction::ThunderCardAction(Character& actor, int damage) :
  CardAction(actor, "PLAYER_SHOOTING"),
  attachmentAnim(actor.GetFirstComponent<AnimationComponent>()->GetFilePath()) {
  ThunderCardAction::damage = damage;

  attachment = new SpriteProxyNode();
  attachment->setTexture(actor.getTexture());
  attachment->SetLayer(-1);
  attachment->EnableParentShader();

  attachmentAnim = Animation(actor.GetFirstComponent<AnimationComponent>()->GetFilePath());
  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("BUSTER");
  AddAttachment(actor, "buster", *attachment).UseAnimation(attachmentAnim);
}

void ThunderCardAction::OnExecute(Character* user) {
  // On shoot frame, drop projectile
  auto onFire = [=]() -> void {
    auto& owner = GetActor();
    Team team = owner.GetTeam();
    auto* thunder = new Thunder(team);
    auto props = thunder->GetHitboxProperties();
    props.damage = damage;
    props.aggressor = user->GetID();
    thunder->SetHitboxProperties(props);

    int step = 1;

    if (team != Team::red) {
      step = -1;
    }

    if (auto tile = owner.GetTile()->Offset(step, 0)) {
      owner.GetField()->AddEntity(*thunder, *tile);
    }
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
}
