#include "bnThunderCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnThunder.h"
#include "bnField.h"

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
    auto& owner = GetCharacter();
    Team team = owner.GetTeam();
    auto* thunder = new Thunder(team);
    auto props = thunder->GetHitboxProperties();
    props.damage = damage;
    props.aggressor = owner.GetID();
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
