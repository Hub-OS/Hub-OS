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

CrackShotCardAction::CrackShotCardAction(Character& actor, int damage) : 
CardAction(actor, "PLAYER_SWORD") {
  CrackShotCardAction::damage = damage;

  attachment = new SpriteProxyNode();
  attachment->setTexture(actor.getTexture());
  attachment->SetLayer(-1);
  attachment->EnableParentShader(true);

  OverrideAnimationFrames({ FRAMES });

  auto* anim = actor.GetFirstComponent<AnimationComponent>();

  if (anim) {
    attachmentAnim = Animation(anim->GetFilePath());
    attachmentAnim.Reload();
    attachmentAnim.SetAnimation("HAND");
  }
}

CrackShotCardAction::~CrackShotCardAction()
{
}

void CrackShotCardAction::OnExecute(Character* user) {
  auto actor = &GetActor();

  // On throw frame, spawn projectile
  auto onThrow = [=]() -> void {
    int step = 1;

    if (actor->GetTeam() == Team::blue) {
      step = -1;
    }

    auto tile = actor->GetTile()->Offset(step, 0);
    auto field = actor->GetField();

    if (tile && tile->IsWalkable()) {
      if (!tile->IsReservedByCharacter()) {
        CrackShot* b = new CrackShot(actor->GetTeam(), tile);
        auto props = b->GetHitboxProperties();
        props.damage = damage;
        props.aggressor = user->GetID();
        b->SetHitboxProperties(props);

        auto direction = (actor->GetTeam() == Team::red) ? Direction::right : Direction::left;
        b->SetDirection(direction);

        actor->GetField()->AddEntity(*b, tile->GetX(), tile->GetY());

        field->AddEntity(*b, *tile);
        Audio().Play(AudioType::TOSS_ITEM_LITE);

        tile->SetState(TileState::broken);
      }
      else {
        tile->SetState(TileState::cracked);
      }

      auto* fx = new ParticleImpact(ParticleImpact::Type::wind);
      field->AddEntity(*fx, *tile);
    }

    Audio().Play(AudioType::TOSS_ITEM_LITE);
  };

  auto addHand = [this] {
    auto owner = &GetActor();
    overlay.setTexture(*owner->getTexture());
    attachment = new SpriteProxyNode(overlay);
    attachment->SetLayer(-1);
    attachment->EnableParentShader(true);

    AddAttachment(*owner, "hilt", *attachment).UseAnimation(attachmentAnim);
  };

  AddAnimAction(2, addHand);
  AddAnimAction(4, onThrow);
}

void CrackShotCardAction::Update(double _elapsed)
{
  CardAction::Update(_elapsed);
}

void CrackShotCardAction::OnAnimationEnd()
{
}

void CrackShotCardAction::OnActionEnd() {
}
