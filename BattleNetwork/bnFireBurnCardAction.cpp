#include "bnFireBurnCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
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

FireBurnCardAction::FireBurnCardAction(Character& user, FireBurn::Type type, int damage) : CardAction(user, "PLAYER_SHOOTING") {
  FireBurnCardAction::damage = damage;
  FireBurnCardAction::type = type;

  attachment = new SpriteProxyNode();
  attachment->setTexture(Textures().LoadTextureFromFile(PATH));
  attachment->SetLayer(-1);

  attachmentAnim = Animation(ANIM);
  attachmentAnim.SetAnimation("DEFAULT");

  auto& userAnim = user.GetFirstComponent<AnimationComponent>()->GetAnimationObject();
  AddAttachment(userAnim, "BUSTER", *attachment).PrepareAnimation(attachmentAnim);

  // add override anims
  OverrideAnimationFrames({ FRAMES });
}

FireBurnCardAction::~FireBurnCardAction()
{
}

void FireBurnCardAction::OnExecute() {
  // On shoot frame, drop projectile
  auto onFire = [this](int offset) -> void {
    auto& owner = GetUser();

    FireBurn* fb = new FireBurn(user.GetField(), user.GetTeam(), type, damage);
    auto props = fb->GetHitboxProperties();
    props.aggressor = &user;
    fb->SetHitboxProperties(props);

    // update node position in the animation
    auto baseOffset = anim->GetPoint("buster").y - anim->GetPoint("origin").y;

    if (baseOffset < 0) { baseOffset = -baseOffset; }

    baseOffset *= 2.0f; // turn into screenspace values (2x scale)

    fb->SetHeight(baseOffset);

    user.GetField()->AddEntity(*fb, user.GetTile()->GetX() + 1 + offset, user.GetTile()->GetY());
  };

  AddAction(2, [onFire]() { onFire(0); });
  AddAction(4, [onFire]() { onFire(1); });
  AddAction(6, [onFire]() { onFire(2); });
}

void FireBurnCardAction::OnEndAction()
{
  GetUser().RemoveNode(attachment);
}
