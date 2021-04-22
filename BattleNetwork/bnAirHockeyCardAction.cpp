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

AirHockeyCardAction::AirHockeyCardAction(Character& actor, int damage) :
  CardAction(actor, "PLAYER_SWORD") {
  AirHockeyCardAction::damage = damage;

  overlay.setTexture(*actor.getTexture());
  attachment = new SpriteProxyNode(overlay);
  attachment->SetLayer(-1);
  attachment->EnableParentShader(true);

  OverrideAnimationFrames({ FRAMES });

  attachmentAnim = Animation(actor.GetFirstComponent<AnimationComponent>()->GetFilePath());
  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("HAND");
}

AirHockeyCardAction::~AirHockeyCardAction()
{
}

void AirHockeyCardAction::OnExecute(Character* user) {
  auto* actor = &GetActor();

  auto onHand = [=] {
    AddAttachment(*actor, "hilt", *attachment).UseAnimation(attachmentAnim);
  };

  // On throw frame, spawn projectile
  auto onThrow = [=]() -> void {

    int step = 1;

    if (actor->GetTeam() == Team::blue) {
      step = -1;
    }

    auto& field = *actor->GetField();
    auto tile = actor->GetTile()->Offset(step, 0);

    if (tile) {
      AirHockey* b = new AirHockey(&field, actor->GetTeam(), this->damage, 10);
      auto props = b->GetHitboxProperties();
      props.aggressor = user->GetID();
      b->SetHitboxProperties(props);

      field.AddEntity(*b, tile->GetX(), tile->GetY());
    }

    if (tile == nullptr) {
      auto* fx = new MobMoveEffect();
      field.AddEntity(*fx, *actor->GetTile());
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
