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

CrackShotCardAction::CrackShotCardAction(Character * owner, int damage) : CardAction(owner, "PLAYER_SWORD", &attachment, "HILT") {
  CrackShotCardAction::damage = damage;

  overlay.setTexture(*owner->getTexture());
  attachment = new SpriteProxyNode(overlay);
  attachment->SetLayer(-1);
  attachment->EnableParentShader(true);

  OverrideAnimationFrames({ FRAMES });

  attachmentAnim = Animation(owner->GetFirstComponent<AnimationComponent>()->GetFilePath());
  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("HAND");
}

CrackShotCardAction::~CrackShotCardAction()
{
}

void CrackShotCardAction::Execute() {
  auto user = GetUser();

  user->AddNode(attachment);
  attachmentAnim.Update(0, attachment->getSprite());

  // On throw frame, spawn projectile
  auto onThrow = [this, user]() -> void {

    auto tile = user->GetField()->GetAt(user->GetTile()->GetX() + 1, user->GetTile()->GetY());

    if (tile && tile->IsWalkable() && !tile->IsReservedByCharacter() && !tile->ContainsEntityType<Character>()) {
      CrackShot* b = new CrackShot(user->GetField(), user->GetTeam(), tile);
      auto props = b->GetHitboxProperties();
      props.damage = damage;
      props.aggressor = user;
      b->SetHitboxProperties(props);

      auto direction = (user->GetTeam() == Team::red) ? Direction::right : Direction::left;
      b->SetDirection(direction);


      user->GetField()->AddEntity(*b, tile->GetX(), tile->GetY());

      AUDIO.Play(AudioType::TOSS_ITEM_LITE);

      tile->SetState(TileState::broken);
    }
  };

  AddAction(3, onThrow);
}

void CrackShotCardAction::OnUpdate(float _elapsed)
{
  attachmentAnim.Update(_elapsed, attachment->getSprite());
  CardAction::OnUpdate(_elapsed);
}

void CrackShotCardAction::EndAction() {
  user->RemoveNode(attachment);
  user->EndCurrentAction();
}
