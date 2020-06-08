#include "bnAirShotCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnAirShot.h"

#define NODE_PATH "resources/spells/AirShot.png"
#define NODE_ANIM "resources/spells/airshot.animation"

#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.05 }

#define FRAMES FRAME1, FRAME2, FRAME3, FRAME3, FRAME3


AirShotCardAction::AirShotCardAction(Character& user, int damage) : CardAction(user, "PLAYER_SHOOTING") {
  AirShotCardAction::damage = damage;

  attachment = new SpriteProxyNode();
  attachment->setTexture(Textures().LoadTextureFromFile(NODE_PATH));
  attachment->SetLayer(-1);

  attachmentAnim = Animation(NODE_ANIM);
  attachmentAnim.SetAnimation("DEFAULT");

  // add override anims
  OverrideAnimationFrames({ FRAMES });
}

void AirShotCardAction::OnExecute() {

  // On shoot frame, drop projectile
  auto onFire = [this]() -> void {
    auto& user = GetUser();

    Audio()().Play(AudioType::SPREADER);

    AirShot* airshot = new AirShot(user.GetField(), user.GetTeam(), damage);
    airshot->SetDirection(Direction::right);
    auto props = airshot->GetHitboxProperties();
    props.aggressor = &user;
    airshot->SetHitboxProperties(props);

    user.GetField()->AddEntity(*airshot, user.GetTile()->GetX() + 1, user.GetTile()->GetY());
  };

  AddAction(2, onFire);
}

AirShotCardAction::~AirShotCardAction()
{
}


void AirShotCardAction::OnEndAction()
{
  auto user = &GetUser();
  user->RemoveNode(attachment);
}
