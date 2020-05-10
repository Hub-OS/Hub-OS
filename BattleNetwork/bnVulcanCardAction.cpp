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


VulcanCardAction::VulcanCardAction(Character& user, int damage) : CardAction(user, "PLAYER_SHOOTING") {
  VulcanCardAction::damage = damage;
  attachment = new SpriteProxyNode();
  attachment->setTexture(TextureResourceManager::GetInstance().LoadTextureFromFile(PATH));
  attachment->SetLayer(-1);

  attachmentAnim = Animation(ANIM);
  attachmentAnim.SetAnimation("DEFAULT");

  Animation& userAnim = user.GetFirstComponent<AnimationComponent>()->GetAnimationObject();
  AddAttachment(userAnim, "BUSTER", *attachment).PrepareAnimation(attachmentAnim);

  // add override anims
  OverrideAnimationFrames({ FRAMES });
}

VulcanCardAction::~VulcanCardAction()
{
}

void VulcanCardAction::OnExecute() {
  // On shoot frame, drop projectile
  auto onFire = [this]() -> void {
    auto& user = GetUser();

    Vulcan* b = new Vulcan(user.GetField(), user.GetTeam(), damage);
    auto props = b->GetHitboxProperties();
    props.aggressor = &user;
    b->SetHitboxProperties(props);
    b->SetDirection(Direction::right);

    GetUser().GetField()->AddEntity(*b, user.GetTile()->GetX() + 1, user.GetTile()->GetY());
  };

  AddAction(2, onFire);
  AddAction(4, onFire);
  AddAction(6, onFire);
}

void VulcanCardAction::OnEndAction()
{
  GetUser().RemoveNode(attachment);
}
