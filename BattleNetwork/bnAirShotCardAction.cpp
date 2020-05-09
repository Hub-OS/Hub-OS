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


AirShotCardAction::AirShotCardAction(Character * owner, int damage) : CardAction(owner, "PLAYER_SHOOTING", &attachment, "Buster"), attachmentAnim(NODE_ANIM) {
  AirShotCardAction::damage = damage;

  airshot.setTexture(*TextureResourceManager::GetInstance().LoadTextureFromFile(NODE_PATH));
  attachment = new SpriteProxyNode(airshot);
  attachment->SetLayer(-1);

  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("DEFAULT");

  // add override anims
  OverrideAnimationFrames({ FRAMES });
}

void AirShotCardAction::Execute() {
  auto user = GetUser();

  user->AddNode(attachment);
  attachmentAnim.Update(0, attachment->getSprite());

  // On shoot frame, drop projectile
  auto onFire = [this, user]() -> void {
    AUDIO.Play(AudioType::SPREADER);

    AirShot* airshot = new AirShot(user->GetField(), user->GetTeam(), damage);
    airshot->SetDirection(Direction::right);
    auto props = airshot->GetHitboxProperties();
    props.aggressor = GetUser();
    airshot->SetHitboxProperties(props);

    user->GetField()->AddEntity(*airshot, user->GetTile()->GetX() + 1, user->GetTile()->GetY());
  };

  AddAction(2, onFire);
}

AirShotCardAction::~AirShotCardAction()
{
}

void AirShotCardAction::OnUpdate(float _elapsed)
{
  attachmentAnim.Update(_elapsed, attachment->getSprite());
  CardAction::OnUpdate(_elapsed);
}

void AirShotCardAction::EndAction()
{
  auto user = GetUser();
  user->RemoveNode(attachment);
  user->EndCurrentAction();
}
