#include "bnAirHockeyCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnAirHockey.h"
#include "bnMobMoveEffect.h"

#define FRAME1 { 1, 0.1666 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.05 }
#define FRAME4 { 4, 0.5 }

#define FRAMES FRAME1, FRAME2, FRAME3, FRAME4

AirHockeyCardAction::AirHockeyCardAction(Character& owner, int damage) :
  CardAction(owner, "PLAYER_SWORD") {
  AirHockeyCardAction::damage = damage;

  overlay.setTexture(*owner.getTexture());
  attachment = new SpriteProxyNode(overlay);
  attachment->SetLayer(-1);
  attachment->EnableParentShader(true);

  OverrideAnimationFrames({ FRAMES });

  attachmentAnim = Animation(owner.GetFirstComponent<AnimationComponent>()->GetFilePath());
  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("HAND");
}

AirHockeyCardAction::~AirHockeyCardAction()
{
}

void AirHockeyCardAction::OnExecute() {
  auto owner = GetOwner();

  auto onHand = [owner, this] {
    AddAttachment(*owner, "hilt", *attachment).UseAnimation(attachmentAnim);
  };

  // On throw frame, spawn projectile
  auto onThrow = [this, owner]() -> void {

    int step = 1;

    if (owner->GetTeam() == Team::blue) {
      step = -1;
    }

    auto tile = GetOwner()->GetField()->GetAt(GetOwner()->GetTile()->GetX() + step, GetOwner()->GetTile()->GetY());

    if (tile) {
      AirHockey* b = new AirHockey(GetOwner()->GetField(), GetOwner()->GetTeam(), this->damage, 10);
      auto props = b->GetHitboxProperties();
      props.aggressor = GetOwnerAs<Character>();
      b->SetHitboxProperties(props);

      GetOwner()->GetField()->AddEntity(*b, tile->GetX(), tile->GetY());
    }

    if (tile == nullptr) {
      auto* fx = new MobMoveEffect(GetOwner()->GetField());
      GetOwner()->GetField()->AddEntity(*fx, *GetOwner()->GetTile());
    }

    Audio().Play(AudioType::TOSS_ITEM_LITE);
  };

  AddAnimAction(2, onHand);
  AddAnimAction(3, onThrow);
}

void AirHockeyCardAction::OnUpdate(double _elapsed)
{
  CardAction::OnUpdate(_elapsed);
}

void AirHockeyCardAction::OnAnimationEnd()
{
}

void AirHockeyCardAction::OnEndAction() {
  Eject();
}
