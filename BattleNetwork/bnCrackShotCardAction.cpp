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
  this->damage = damage;

  overlay.setTexture(*owner->getTexture());
  this->attachment = new SpriteProxyNode(overlay);
  this->attachment->SetLayer(-1);
  this->attachment->EnableParentShader(true);

  this->OverrideAnimationFrames({ FRAMES });

  attachmentAnim = Animation(owner->GetFirstComponent<AnimationComponent>()->GetFilePath());
  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("HAND");
}

CrackShotCardAction::~CrackShotCardAction()
{
}

void CrackShotCardAction::Execute() {
  auto owner = GetOwner();

  owner->AddNode(this->attachment);
  attachmentAnim.Update(0, this->attachment->getSprite());

  // On throw frame, spawn projectile
  auto onThrow = [this, owner]() -> void {

    auto tile = GetOwner()->GetField()->GetAt(GetOwner()->GetTile()->GetX() + 1, GetOwner()->GetTile()->GetY());

    if (tile && tile->IsWalkable() && !tile->IsReservedByCharacter() && !tile->ContainsEntityType<Character>()) {
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

void CrackShotCardAction::OnUpdate(float _elapsed)
{
  attachmentAnim.Update(_elapsed, this->attachment->getSprite());
  CardAction::OnUpdate(_elapsed);
}

void CrackShotCardAction::EndAction() {
  this->GetOwner()->RemoveNode(attachment);
  GetOwner()->FreeComponentByID(this->GetID());
  delete this;
}
