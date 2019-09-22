#include "bnCrackShotChipAction.h"
#include "bnChipAction.h"
#include "bnSpriteSceneNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnCrackShot.h"

#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.3 }

#define FRAMES FRAME1, FRAME2, FRAME3

CrackShotChipAction::CrackShotChipAction(Character * owner, int damage) : ChipAction(owner, "PLAYER_SWORD", &attachment, "BUSTER") {
  overlay.setTexture(*owner->getTexture());
  this->attachment = new SpriteSceneNode(overlay);
  this->attachment->SetLayer(-1);
  owner->AddNode(this->attachment);

  this->OverrideAnimationFrames({ FRAMES });

  attachmentAnim = Animation(owner->GetFirstComponent<AnimationComponent>()->GetFilePath());
  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("HAND");
  attachmentAnim.Update(0, *this->attachment);

  // On throw frame, spawn projectile
  auto onThrow = [this, damage, owner]() -> void {

    auto tile = GetOwner()->GetField()->GetAt(GetOwner()->GetTile()->GetX() + 1, GetOwner()->GetTile()->GetY());

    if (tile && tile->IsWalkable() && !tile->IsReservedByCharacter()) {
      CrackShot* b = new CrackShot(GetOwner()->GetField(), GetOwner()->GetTeam(), tile);
      auto props = b->GetHitboxProperties();
      props.damage = damage;
      props.aggressor = GetOwnerAs<Character>();
      b->SetHitboxProperties(props);

      auto direction = (owner->GetTeam() == Team::RED) ? Direction::RIGHT : Direction::LEFT;
      b->SetDirection(direction);


      GetOwner()->GetField()->AddEntity(*b, tile->GetX(), tile->GetY());

      AUDIO.Play(AudioType::TOSS_ITEM_LITE);

      tile->SetState(TileState::BROKEN);
    }
  };

  this->AddAction(3, onThrow);
}

CrackShotChipAction::~CrackShotChipAction()
{
}

void CrackShotChipAction::OnUpdate(float _elapsed)
{
  attachmentAnim.Update(_elapsed, *this->attachment);
  ChipAction::OnUpdate(_elapsed);
}

void CrackShotChipAction::EndAction() {
  this->GetOwner()->RemoveNode(attachment);
  GetOwner()->FreeComponentByID(this->GetID());
  delete this;
}
