#include "bnCrackShotCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnParticleImpact.h"
#include "bnCrackShot.h"

#define FRAME1 { 1, 0.1666 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.05 }
#define FRAME4 { 4, 0.5 }

#define FRAMES FRAME1, FRAME2, FRAME3, FRAME4

CrackShotCardAction::CrackShotCardAction(Character& user, int damage) : 
CardAction(user, "PLAYER_SWORD") {
  CrackShotCardAction::damage = damage;

  attachment = new SpriteProxyNode();
  attachment->setTexture(user.getTexture());
  attachment->SetLayer(-1);
  attachment->EnableParentShader(true);

  OverrideAnimationFrames({ FRAMES });

  auto* anim = user.GetFirstComponent<AnimationComponent>();

  if (anim) {
    attachmentAnim = Animation(anim->GetFilePath());
    attachmentAnim.Reload();
    attachmentAnim.SetAnimation("HAND");

    AddAttachment(anim->GetAnimationObject(), "hilt", *attachment).UseAnimation(attachmentAnim);
  }
}

CrackShotCardAction::~CrackShotCardAction()
{
}

void CrackShotCardAction::OnExecute() {
  auto owner = GetOwner();

  // On throw frame, spawn projectile
  auto onThrow = [this, owner]() -> void {
    int step = 1;

    if (owner->GetTeam() == Team::blue) {
      step = -1;
    }

    auto tile = GetOwner()->GetField()->GetAt(GetOwner()->GetTile()->GetX() + step, GetOwner()->GetTile()->GetY());

    if (tile && tile->IsWalkable() && !tile->IsReservedByCharacter()) {
      CrackShot* b = new CrackShot(owner->GetTeam(), tile);
      auto props = b->GetHitboxProperties();
      props.damage = damage;
      props.aggressor = owner;
      b->SetHitboxProperties(props);

      auto direction = (owner->GetTeam() == Team::red) ? Direction::right : Direction::left;
      b->SetDirection(direction);

      owner->GetField()->AddEntity(*b, tile->GetX(), tile->GetY());

      GetOwner()->GetField()->AddEntity(*b, tile->GetX(), tile->GetY());
      Audio().Play(AudioType::TOSS_ITEM_LITE);

      tile->SetState(TileState::broken);
    }

    if (tile) {
      auto* fx = new ParticleImpact(ParticleImpact::Type::wind);
      GetOwner()->GetField()->AddEntity(*fx, tile->GetX(), tile->GetY());
    }

    Audio().Play(AudioType::TOSS_ITEM_LITE);
  };

  auto addHand = [this] {
    auto* owner = GetOwner();
    overlay.setTexture(*owner->getTexture());
    attachment = new SpriteProxyNode(overlay);
    attachment->SetLayer(-1);
    attachment->EnableParentShader(true);

    AddAttachment(*owner, "hilt", *attachment).UseAnimation(attachmentAnim);
  };

  AddAnimAction(2, addHand);
  AddAnimAction(4, onThrow);
}

void CrackShotCardAction::OnUpdate(double _elapsed)
{
  CardAction::OnUpdate(_elapsed);
}

void CrackShotCardAction::OnAnimationEnd()
{
}

void CrackShotCardAction::OnEndAction() {
  Eject();
}
