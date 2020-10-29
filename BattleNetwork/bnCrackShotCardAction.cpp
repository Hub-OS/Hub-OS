#include "bnCrackShotCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnCrackShot.h"

#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.3 }

#define FRAMES FRAME1, FRAME2, FRAME3

CrackShotCardAction::CrackShotCardAction(Character * owner, int damage) 
  : 
  CardAction(*owner, "PLAYER_SWORD") {
  CrackShotCardAction::damage = damage;

  overlay.setTexture(*owner->getTexture());
  attachment = new SpriteProxyNode(overlay);
  attachment->SetLayer(-1);
  attachment->EnableParentShader(true);

  AddAttachment(*owner, "hilt", *attachment).UseAnimation(attachmentAnim);

  OverrideAnimationFrames({ FRAMES });

  attachmentAnim = Animation(owner->GetFirstComponent<AnimationComponent>()->GetFilePath());
  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("HAND");
}

CrackShotCardAction::~CrackShotCardAction()
{
}

void CrackShotCardAction::Execute() {
  auto owner = GetOwner();

  // On throw frame, spawn projectile
  auto onThrow = [this, owner]() -> void {

    int step = 1;

    if (owner->GetTeam() == Team::blue) {
      step = -1;
    }

    auto tile = GetOwner()->GetField()->GetAt(GetOwner()->GetTile()->GetX() + step, GetOwner()->GetTile()->GetY());

    if (tile && tile->IsWalkable() && !tile->IsReservedByCharacter() && !tile->ContainsEntityType<Character>()) {
      CrackShot* b = new CrackShot(GetOwner()->GetField(), GetOwner()->GetTeam(), tile);
      auto props = b->GetHitboxProperties();
      props.damage = damage;
      props.aggressor = GetOwnerAs<Character>();
      b->SetHitboxProperties(props);

      auto direction = (owner->GetTeam() == Team::red) ? Direction::right : Direction::left;
      b->SetDirection(direction);


      GetOwner()->GetField()->AddEntity(*b, tile->GetX(), tile->GetY());

      AUDIO.Play(AudioType::TOSS_ITEM_LITE);

      tile->SetState(TileState::broken);
    }
  };

  AddAnimAction(3, onThrow);
}

void CrackShotCardAction::OnUpdate(float _elapsed)
{
  CardAction::OnUpdate(_elapsed);
}

void CrackShotCardAction::OnAnimationEnd()
{
}

void CrackShotCardAction::EndAction() {
  Eject();
}
