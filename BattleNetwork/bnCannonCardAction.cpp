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


CannonCardAction::CannonCardAction(Character& owner, int damage, CannonCardAction::Type type) : CardAction(owner, "PLAYER_SHOOTING") {
  CannonCardAction::damage = damage;

  attachment = new SpriteProxyNode();
  attachment->setTexture(Textures().LoadTextureFromFile(CANNON_PATH));
  attachment->SetLayer(-1);

  attachmentAnim = Animation(CANNON_ANIM);

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

  AddAttachment(anim->GetAnimationObject(), "buster", *attachment).PrepareAnimation(attachmentAnim);
}

CannonCardAction::~CannonCardAction()
{
}

void CannonCardAction::OnExecute() {

  // On shoot frame, drop projectile
  auto onFire = [this]() -> void {
    auto& user = GetUser();

    // Spawn a single cannon instance on the tile in front of the player
    Cannon* cannon = new Cannon(user.GetField(), user.GetTeam(), damage);
    auto props = cannon->GetHitboxProperties();
    props.aggressor = &GetUser();
    cannon->SetHitboxProperties(props);

    Audio().Play(AudioType::CANNON);

    cannon->SetDirection(Direction::right);

    user.GetField()->AddEntity(*cannon, user.GetTile()->GetX() + 1, user.GetTile()->GetY());
  };

  AddAction(6, onFire);
}

void CannonCardAction::OnEndAction()
{
  GetUser().RemoveNode(attachment);
}
