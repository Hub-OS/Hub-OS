#include "bnFireBurnChipAction.h"
#include "bnChipAction.h"
#include "bnSpriteSceneNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

#define PATH "resources/spells/buster_flame.png"
#define ANIM "resources/spells/buster_flame.animation"

#define WAIT   { 1, 0.15 }
#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.05 }
#define FRAME4 { 4, 0.05 }
#define FRAME5 { 5, 0.05 }

// TODO: check frame-by-frame anim
#define FRAMES WAIT, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1


FireBurnChipAction::FireBurnChipAction(Character * owner, FireBurn::Type type, int damage) : ChipAction(owner, "PLAYER_SHOOTING", &attachment, "Buster"), attachmentAnim(ANIM) {
  overlay.setTexture(*TextureResourceManager::GetInstance().LoadTextureFromFile(PATH));
  this->attachment = new SpriteSceneNode(overlay);
  this->attachment->SetLayer(-1);
  owner->AddNode(this->attachment);

  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("DEFAULT");
  attachmentAnim.Update(0, *this->attachment);

  // add override anims
  this->OverrideAnimationFrames({ FRAMES });

  // On shoot frame, drop projectile
  auto onFire = [this, damage, owner, type](int offset) -> void {
    FireBurn* fb = new FireBurn(GetOwner()->GetField(), GetOwner()->GetTeam(), type, damage);
    auto props = fb->GetHitboxProperties();
    props.aggressor = GetOwnerAs<Character>();
    fb->SetHitboxProperties(props);

    GetOwner()->GetField()->AddEntity(*fb, GetOwner()->GetTile()->GetX() + 1 + offset, GetOwner()->GetTile()->GetY());
  };


  this->AddAction(2, [onFire]() { onFire(0); });
  this->AddAction(4, [onFire]() { onFire(1); });
  this->AddAction(6, [onFire]() { onFire(2); });
}

FireBurnChipAction::~FireBurnChipAction()
{
}

void FireBurnChipAction::OnUpdate(float _elapsed)
{
  attachmentAnim.Update(_elapsed, *this->attachment);
  ChipAction::OnUpdate(_elapsed);
}

void FireBurnChipAction::EndAction()
{
  this->GetOwner()->RemoveNode(attachment);
  GetOwner()->FreeComponentByID(this->GetID());
  delete this;
}
