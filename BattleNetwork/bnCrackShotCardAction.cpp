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

CrackShotCardAction::CrackShotCardAction(Character& user, int damage) : CardAction(user, "PLAYER_SWORD") {
  CrackShotCardAction::damage = damage;

  attachment = new SpriteProxyNode();
  attachment->setTexture(user.getTexture());
  attachment->SetLayer(-1);
  attachment->EnableParentShader(true);

  OverrideAnimationFrames({ FRAMES });

  attachmentAnim = Animation(anim->GetFilePath());
  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("HAND");

  AddAttachment(anim->GetAnimationObject(), "hilt", *attachment).PrepareAnimation(attachmentAnim);
}

CrackShotCardAction::~CrackShotCardAction()
{
}

void CrackShotCardAction::OnExecute() {
  // On throw frame, spawn projectile
  auto onThrow = [this]() -> void {
    auto& user = GetUser();

    auto tile = user.GetField()->GetAt(user.GetTile()->GetX() + 1, user.GetTile()->GetY());

    if (tile && tile->IsWalkable() && !tile->IsReservedByCharacter()) {
      CrackShot* b = new CrackShot(user.GetField(), user.GetTeam(), tile);
      auto props = b->GetHitboxProperties();
      props.damage = damage;
      props.aggressor = &user;
      b->SetHitboxProperties(props);

      auto direction = (user.GetTeam() == Team::red) ? Direction::right : Direction::left;
      b->SetDirection(direction);

      user.GetField()->AddEntity(*b, tile->GetX(), tile->GetY());

      AUDIO.Play(AudioType::TOSS_ITEM_LITE);

      tile->SetState(TileState::broken);
    }
  };

  AddAction(3, onThrow);
}

void CrackShotCardAction::OnEndAction() {
  GetUser().RemoveNode(attachment);
}
