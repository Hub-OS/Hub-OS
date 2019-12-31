#include "bnCannonChipAction.h"
#include "bnChipAction.h"
#include "bnSpriteSceneNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnCannon.h"

#define CANNON_PATH "resources/spells/CannonSeries.png"
#define CANNON_ANIM "resources/spells/cannon.animation"

#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.05 }

#define FRAMES FRAME1, FRAME1, FRAME1, FRAME1, FRAME2, FRAME3, FRAME3, FRAME3, FRAME3


CannonChipAction::CannonChipAction(Character * owner, int damage, CannonChipAction::Type type) : ChipAction(owner, "PLAYER_SHOOTING", &attachment, "Buster"), attachmentAnim(CANNON_ANIM) {
  this->damage = damage;

  cannon.setTexture(*TextureResourceManager::GetInstance().LoadTextureFromFile(CANNON_PATH));
  this->attachment = new SpriteSceneNode(cannon);
  this->attachment->SetLayer(-1);

  attachmentAnim.Reload();

  switch (type) {
  case Type::green:
    attachmentAnim.SetAnimation("Cannon1");
    break;
  case Type::blue:
    attachmentAnim.SetAnimation("Cannon2");
    break;
  default:
    attachmentAnim.SetAnimation("Cannon3");

  }

  // add override anims
  this->OverrideAnimationFrames({ FRAMES });
}

CannonChipAction::~CannonChipAction()
{
}

void CannonChipAction::Execute() {
  auto owner = GetOwner();
  owner->AddNode(this->attachment);

  attachmentAnim.Update(0, *this->attachment);

  // On shoot frame, drop projectile
  auto onFire = [this, owner]() -> void {
    // Spawn a single cannon instance on the tile in front of the player
    Cannon* cannon = new Cannon(GetOwner()->GetField(), GetOwner()->GetTeam(), damage);
    auto props = cannon->GetHitboxProperties();
    props.aggressor = GetOwnerAs<Character>();
    cannon->SetHitboxProperties(props);

    AUDIO.Play(AudioType::CANNON);

    cannon->SetDirection(Direction::RIGHT);

    GetOwner()->GetField()->AddEntity(*cannon, GetOwner()->GetTile()->GetX() + 1, GetOwner()->GetTile()->GetY());
  };

  this->AddAction(6, onFire);
}

void CannonChipAction::OnUpdate(float _elapsed)
{
  attachmentAnim.Update(_elapsed, *this->attachment);
  ChipAction::OnUpdate(_elapsed);
}

void CannonChipAction::EndAction()
{
  this->GetOwner()->RemoveNode(attachment);
  GetOwner()->FreeComponentByID(this->GetID());
  delete this;
}
