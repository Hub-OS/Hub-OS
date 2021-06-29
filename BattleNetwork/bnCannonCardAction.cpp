#include "bnCannonCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnCannon.h"
#include "bnField.h"

#define CANNON_PATH "resources/spells/CannonSeries.png"
#define CANNON_ANIM "resources/spells/cannon.animation"

#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.05 }

#define FRAMES FRAME1, FRAME1, FRAME1, FRAME1, FRAME2, FRAME3, FRAME3, FRAME3, FRAME3

CannonCardAction::CannonCardAction(Character& actor, CannonCardAction::Type type, int damage) :
  CardAction(actor, "PLAYER_SHOOTING"),
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
  AddAttachment(actor, "buster", *attachment).UseAnimation(attachmentAnim);
}

CannonCardAction::~CannonCardAction()
{
}

void CannonCardAction::OnExecute(Character* user) {
  // On shoot frame, drop projectile
  auto onFire = [=]() -> void {
    // Spawn a single cannon instance on the tile in front of the player
    Team team = user->GetTeam();
    Cannon* cannon = new Cannon(team, damage);
    auto props = cannon->GetHitboxProperties();
    props.aggressor = user->GetID();

    cannon->SetHitboxProperties(props);

    Audio().Play(AudioType::CANNON);

    if (team == Team::red) {
      cannon->SetDirection(Direction::right);
    }
    else {
      cannon->SetDirection(Direction::left);
    }

    user->GetField()->AddEntity(*cannon, user->GetCurrentTile()->GetX() + 1, user->GetTile()->GetY());
  };

  AddAnimAction(6, onFire);
}

void CannonCardAction::Update(double _elapsed)
{
  CardAction::Update(_elapsed);
}

void CannonCardAction::OnAnimationEnd()
{
}

void CannonCardAction::OnActionEnd()
{
}
