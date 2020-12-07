#include "bnCannonCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnCannon.h"

#define CANNON_PATH "resources/spells/CannonSeries.png"
#define CANNON_ANIM "resources/spells/cannon.animation"

#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.05 }

#define FRAMES FRAME1, FRAME1, FRAME1, FRAME1, FRAME2, FRAME3, FRAME3, FRAME3, FRAME3

CannonCardAction::CannonCardAction(Character * owner, int damage, CannonCardAction::Type type) : 
  CardAction(*owner, "PLAYER_SHOOTING"), 
  attachmentAnim(CANNON_ANIM),  
  type(type) {
  CannonCardAction::damage = damage;
  
  attachment = new SpriteProxyNode();
  attachment->setTexture(Textures().LoadTextureFromFile(CANNON_PATH));
  attachment->SetLayer(-1);

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
  OverrideAnimationFrames({ FRAMES });
  AddAttachment(*owner, "buster", *attachment).UseAnimation(attachmentAnim);
}

CannonCardAction::~CannonCardAction()
{
}

void CannonCardAction::OnExecute() {
  // On shoot frame, drop projectile
  auto onFire = [this]() -> void {
    // Spawn a single cannon instance on the tile in front of the player
    Team team = GetOwner()->GetTeam();
    Cannon* cannon = new Cannon(GetOwner()->GetField(), team, damage);
    auto props = cannon->GetHitboxProperties();
    props.aggressor = GetOwner();

    cannon->SetHitboxProperties(props);

    Audio().Play(AudioType::CANNON);

    if (team == Team::red) {
      cannon->SetDirection(Direction::right);
    }
    else {
      cannon->SetDirection(Direction::left);
    }

    user.GetField()->AddEntity(*cannon, user.GetTile()->GetX() + 1, user.GetTile()->GetY());
  };

  AddAnimAction(6, onFire);
}

void CannonCardAction::OnUpdate(float _elapsed)
{
  CardAction::OnUpdate(_elapsed);
}

void CannonCardAction::OnAnimationEnd()
{
}

void CannonCardAction::EndAction()
{
  Eject();
}
