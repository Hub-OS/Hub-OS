#include "bnVulcanCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnVulcan.h"

#define PATH "resources/spells/spell_vulcan.png"
#define ANIM "resources/spells/spell_vulcan.animation"

#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.05 }

// TODO: check frame-by-frame anim
#define FRAMES FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1


VulcanCardAction::VulcanCardAction(Character * user, int damage) : CardAction(user, "PLAYER_SHOOTING", &attachment, "Buster"), attachmentAnim(ANIM) {
  VulcanCardAction::damage = damage;
  overlay.setTexture(*TextureResourceManager::GetInstance().LoadTextureFromFile(PATH));
  attachment = new SpriteProxyNode(overlay);
  attachment->SetLayer(-1);
  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("DEFAULT");

  // add override anims
  OverrideAnimationFrames({ FRAMES });
}

VulcanCardAction::~VulcanCardAction()
{
}

void VulcanCardAction::Execute() {
  auto user = GetUser();

  user->AddNode(attachment);
  attachmentAnim.Update(0, attachment->getSprite());

  // On shoot frame, drop projectile
  auto onFire = [this, user]() -> void {
    Vulcan* b = new Vulcan(GetUser()->GetField(), GetUser()->GetTeam(), damage);
    auto props = b->GetHitboxProperties();
    props.aggressor = user;
    b->SetHitboxProperties(props);
    b->SetDirection(Direction::right);

    GetUser()->GetField()->AddEntity(*b, GetUser()->GetTile()->GetX() + 1, GetUser()->GetTile()->GetY());
  };


  AddAction(2, onFire);
  AddAction(4, onFire);
  AddAction(6, onFire);
}

void VulcanCardAction::OnUpdate(float _elapsed)
{
  attachmentAnim.Update(_elapsed, attachment->getSprite());
  CardAction::OnUpdate(_elapsed);
}

void VulcanCardAction::EndAction()
{
  GetUser()->RemoveNode(attachment);
  GetUser()->EndCurrentAction();
}
