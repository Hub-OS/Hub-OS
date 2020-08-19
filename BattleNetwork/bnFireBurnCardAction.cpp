#include "bnFireBurnCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

#define PATH "resources/spells/buster_flame.png"
#define ANIM "resources/spells/buster_flame.animation"

#define WAIT   { 1, 0.05 }
#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.05 }

// TODO: check frame-by-frame anim
#define FRAMES WAIT, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1

FireBurnCardAction::FireBurnCardAction(Character * owner, FireBurn::Type type, int damage) : CardAction(owner, "PLAYER_SHOOTING", &attachment, "Buster"), attachmentAnim(ANIM) {
  FireBurnCardAction::damage = damage;
  FireBurnCardAction::type = type;

  overlay.setTexture(*TextureResourceManager::GetInstance().LoadTextureFromFile(PATH));
  attachment = new SpriteProxyNode(overlay);
  attachment->SetLayer(-1);
  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("DEFAULT");

  // add override anims
  OverrideAnimationFrames({ FRAMES });
}

FireBurnCardAction::~FireBurnCardAction()
{
}

void FireBurnCardAction::Execute() {
  auto owner = GetOwner();

  owner->AddNode(attachment);
  attachmentAnim.Update(0, attachment->getSprite());

  // On shoot frame, drop projectile
  auto onFire = [this, owner](int offset) -> void {
    Team team = GetOwner()->GetTeam();
    FireBurn* fb = new FireBurn(GetOwner()->GetField(), team, type, damage);
    auto props = fb->GetHitboxProperties();
    props.aggressor = GetOwnerAs<Character>();
    fb->SetHitboxProperties(props);

    // update node position in the animation
    auto baseOffset = anim->GetPoint(nodeName).y - anim->GetPoint("origin").y;

    if (baseOffset < 0) { baseOffset = -baseOffset; }


    baseOffset *= 2.0f;

    fb->SetHeight(baseOffset);

    int dir = team == Team::red ? 1 : -1;

    GetOwner()->GetField()->AddEntity(*fb, GetOwner()->GetTile()->GetX() + ((1 + offset)*dir), GetOwner()->GetTile()->GetY());
  };


  AddAction(2, [onFire]() { onFire(0); });
  AddAction(4, [onFire]() { onFire(1); });
  AddAction(6, [onFire]() { onFire(2); });
}

void FireBurnCardAction::OnUpdate(float _elapsed)
{
  attachmentAnim.Update(_elapsed, attachment->getSprite());
  CardAction::OnUpdate(_elapsed);
}

void FireBurnCardAction::OnAnimationEnd()
{
}

void FireBurnCardAction::EndAction()
{
  GetOwner()->RemoveNode(attachment);
  GetOwner()->FreeComponentByID(GetID());
  delete this;
}
