#include "bnVulcanCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnVulcan.h"

#define PATH "resources/spells/spell_vulcan.png"
#define ANIM "resources/spells/spell_vulcan.animation"

#define FRAME1 { 1, 0.1 }
#define FRAME2 { 2, 0.1 }
#define FRAME3 { 3, 0.1 }

// TODO: check frame-by-frame anim
#define FRAMES FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1


VulcanCardAction::VulcanCardAction(Character * owner, int damage) : 
  CardAction(*owner, "PLAYER_SHOOTING"), attachmentAnim(ANIM) {
  VulcanCardAction::damage = damage;
  overlay.setTexture(*TextureResourceManager::GetInstance().LoadTextureFromFile(PATH));
  attachment = new SpriteProxyNode(overlay);
  attachment->SetLayer(-1);
  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("DEFAULT");

  // add override anims
  OverrideAnimationFrames({ FRAMES });

  AddAttachment(*owner, "buster", *attachment).UseAnimation(attachmentAnim);
}

VulcanCardAction::~VulcanCardAction()
{
}

void VulcanCardAction::Execute() {
  auto owner = GetOwner();

  // On shoot frame, drop projectile
  auto onFire = [this, owner]() -> void {
    Team team = GetOwner()->GetTeam();
    Vulcan* b = new Vulcan(GetOwner()->GetField(), team, damage);
    auto props = b->GetHitboxProperties();
    props.aggressor = GetOwnerAs<Character>();
    b->SetHitboxProperties(props);

    int step = 1;

    if (team == Team::red) {
      b->SetDirection(Direction::right);
    }
    else {
      step = -1;
      b->SetDirection(Direction::left);
    }

    GetOwner()->GetField()->AddEntity(*b, GetOwner()->GetTile()->GetX() + step, GetOwner()->GetTile()->GetY());
  };


  AddAnimAction(2, onFire);
  AddAnimAction(4, onFire);
  AddAnimAction(6, onFire);
}

void VulcanCardAction::OnUpdate(float _elapsed)
{
  CardAction::OnUpdate(_elapsed);
}

void VulcanCardAction::OnAnimationEnd()
{
}

void VulcanCardAction::EndAction()
{
  Eject();
}
