#include "bnFireBurnCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"

#define PATH "resources/spells/buster_flame.png"
#define ANIM "resources/spells/buster_flame.animation"

#define WAIT   { 1, 0.05 }
#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.05 }

// TODO: check frame-by-frame anim
#define FRAMES  WAIT,   FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, \
                FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, \
                FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1

FireBurnCardAction::FireBurnCardAction(Character& owner, FireBurn::Type type, int damage) 
  : 
  CardAction(owner, "PLAYER_SHOOTING"),
  attachmentAnim(ANIM) {
  FireBurnCardAction::damage = damage;
  FireBurnCardAction::type = type;

  attachment = new SpriteProxyNode();
  attachment->setTexture(Textures().LoadTextureFromFile(PATH));
  attachment->SetLayer(-1);

  attachmentAnim = Animation(ANIM);
  attachmentAnim.SetAnimation("DEFAULT");

  // add override anims
  OverrideAnimationFrames({ FRAMES });

  AddAttachment(owner, "buster", *attachment).UseAnimation(attachmentAnim);
}

FireBurnCardAction::~FireBurnCardAction()
{
}
void FireBurnCardAction::OnExecute() {
  auto owner = &GetCharacter();

  // On shoot frame, drop projectile
  auto onFire = [this, owner](int offset) -> void {
    Team team = owner->GetTeam();
    FireBurn* fb = new FireBurn(team, type, damage);
    auto props = fb->GetHitboxProperties();
    props.aggressor = owner;
    fb->SetHitboxProperties(props);
    fb->CrackTiles(crackTiles);

    // update node position in the animation
    auto baseOffset = CalculatePointOffset("buster");

    baseOffset *= 2.0f;

    fb->SetHeight(-baseOffset.y);

    int dir = team == Team::red ? 1 : -1;

    auto tile = owner->GetTile()->Offset(((1 + offset) * dir), 0);

    if (tile) {
      owner->GetField()->AddEntity(*fb, *tile);
    }
  };


  AddAnimAction(2, [onFire]() { onFire(0); });
  AddAnimAction(4, [onFire]() { onFire(1); });
  AddAnimAction(6, [onFire]() { onFire(2); });
}

void FireBurnCardAction::CrackTiles(bool state)
{
  crackTiles = state;
}

void FireBurnCardAction::Update(double _elapsed)
{
  CardAction::Update(_elapsed);
}

void FireBurnCardAction::OnAnimationEnd()
{
  GetCharacter().RemoveNode(attachment);
}

void FireBurnCardAction::OnEndAction()
{
  OnAnimationEnd();
}