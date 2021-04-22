#include "bnVulcanCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnVulcan.h"
#include "bnField.h"

#define PATH "resources/spells/spell_vulcan.png"
#define ANIM "resources/spells/spell_vulcan.animation"

#define FRAME1 { 1, 0.1 }
#define FRAME2 { 2, 0.1 }
#define FRAME3 { 3, 0.1 }

// TODO: check frame-by-frame anim
#define FRAMES FRAME1, FRAME2, FRAME1, FRAME2, FRAME1, FRAME2, FRAME1

VulcanCardAction::VulcanCardAction(Character& actor, int damage) : 
  CardAction(actor, "PLAYER_SHOOTING"), attachmentAnim(ANIM) {
  VulcanCardAction::damage = damage;
  attachment = new SpriteProxyNode();
  attachment->setTexture(Textures().LoadTextureFromFile(PATH));
  attachment->SetLayer(-1);

  attachmentAnim = Animation(ANIM);
  attachmentAnim.SetAnimation("DEFAULT");

  Animation& userAnim = actor.GetFirstComponent<AnimationComponent>()->GetAnimationObject();
  AddAttachment(userAnim, "BUSTER", *attachment).UseAnimation(attachmentAnim);

  // add override anims
  OverrideAnimationFrames({ FRAMES });

  AddAttachment(actor, "buster", *attachment).UseAnimation(attachmentAnim);
}

VulcanCardAction::~VulcanCardAction()
{
}
void VulcanCardAction::OnExecute(Character* user) {

  // On shoot frame, drop projectile
  auto onFire = [=]() -> void {
    Team team = user->GetTeam();
    Vulcan* b = new Vulcan(team, damage);
    auto props = b->GetHitboxProperties();
    props.aggressor = user->GetID();
    b->SetHitboxProperties(props);

    int step = 1;

    if (team == Team::red) {
      b->SetDirection(Direction::right);
    }
    else {
      step = -1;
      b->SetDirection(Direction::left);
    }

    if (auto tile = user->GetTile()->Offset(step, 0)) {
      GetActor().GetField()->AddEntity(*b, *tile);
    }
  };


  AddAnimAction(2, onFire);
  AddAnimAction(4, onFire);
  AddAnimAction(6, onFire);
}

void VulcanCardAction::Update(double _elapsed)
{
  CardAction::Update(_elapsed);
}

void VulcanCardAction::OnAnimationEnd()
{
}

void VulcanCardAction::OnEndAction()
{
}
