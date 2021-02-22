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
  auto* owner = &GetCharacter();

  auto onHand = [owner, this] {
    AddAttachment(*owner, "hilt", *attachment).UseAnimation(attachmentAnim);
  };

  // On throw frame, spawn projectile
  auto onThrow = [this, owner]() -> void {

    int step = 1;

    if (owner->GetTeam() == Team::blue) {
      step = -1;
    }

    auto& field = *owner->GetField();
    auto tile = owner->GetTile()->Offset(step, 0);

    if (tile) {
      AirHockey* b = new AirHockey(&field, owner->GetTeam(), this->damage, 10);
      auto props = b->GetHitboxProperties();
      props.aggressor = &GetCharacter();
      b->SetHitboxProperties(props);

      field.AddEntity(*b, tile->GetX(), tile->GetY());
    }

    if (tile == nullptr) {
      auto* fx = new MobMoveEffect();
      field.AddEntity(*fx, *owner->GetTile());
    }

    Audio().Play(AudioType::TOSS_ITEM_LITE);
  };

  AddAnimAction(2, onHand);
  AddAnimAction(3, onThrow);
}

void AirHockeyCardAction::Update(double _elapsed)
{
  CardAction::Update(_elapsed);
}

void AirHockeyCardAction::OnAnimationEnd()
{
}

void AirHockeyCardAction::OnEndAction() {
}
