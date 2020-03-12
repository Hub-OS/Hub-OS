#include "bnFireBurnCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteSceneNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

#define PATH "resources/spells/buster_flame.png"
#define ANIM "resources/spells/buster_flame.animation"

#define WAIT   { 1, 0.15 }
#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.05 }

// TODO: check frame-by-frame anim
#define FRAMES WAIT, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1

FireBurnCardAction::FireBurnCardAction(Character * owner, FireBurn::Type type, int damage) : CardAction(owner, "PLAYER_SHOOTING", &attachment, "Buster"), attachmentAnim(ANIM) {
  this->damage = damage;
  this->type = type;

  overlay.setTexture(*TextureResourceManager::GetInstance().LoadTextureFromFile(PATH));
  this->attachment = new SpriteSceneNode(overlay);
  this->attachment->SetLayer(-1);
  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("DEFAULT");

  // add override anims
  this->OverrideAnimationFrames({ FRAMES });
}

FireBurnCardAction::~FireBurnCardAction()
{
}

void FireBurnCardAction::Execute() {
  auto owner = GetOwner();

  owner->AddNode(this->attachment);
  attachmentAnim.Update(0, *this->attachment);

  // On shoot frame, drop projectile
  auto onFire = [this, owner](int offset) -> void {
    FireBurn* fb = new FireBurn(GetOwner()->GetField(), GetOwner()->GetTeam(), type, damage);
    auto props = fb->GetHitboxProperties();
    props.aggressor = GetOwnerAs<Character>();
    fb->SetHitboxProperties(props);

    // update node position in the animation
    auto baseOffset = anim->GetPoint(nodeName).y - anim->GetPoint("origin").y;

    if (baseOffset < 0) { baseOffset = -baseOffset; }


    baseOffset *= 2.0f;

    fb->SetHeight(baseOffset);

    GetOwner()->GetField()->AddEntity(*fb, GetOwner()->GetTile()->GetX() + 1 + offset, GetOwner()->GetTile()->GetY());
  };


  this->AddAction(2, [onFire]() { onFire(0); });
  this->AddAction(4, [onFire]() { onFire(1); });
  this->AddAction(6, [onFire]() { onFire(2); });
}

void FireBurnCardAction::OnUpdate(float _elapsed)
{
  attachmentAnim.Update(_elapsed, *this->attachment);
  CardAction::OnUpdate(_elapsed);
}

void FireBurnCardAction::EndAction()
{
  this->GetOwner()->RemoveNode(attachment);
  GetOwner()->FreeComponentByID(this->GetID());
  delete this;
}
