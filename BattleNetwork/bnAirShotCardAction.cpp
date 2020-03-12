#include "bnAirShotCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteSceneNode.h"
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
  this->damage = damage;

  airshot.setTexture(*TextureResourceManager::GetInstance().LoadTextureFromFile(NODE_PATH));
  this->attachment = new SpriteSceneNode(airshot);
  this->attachment->SetLayer(-1);

  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("DEFAULT");

  // add override anims
  this->OverrideAnimationFrames({ FRAMES });
}

void AirShotCardAction::Execute() {
  auto owner = GetOwner();

  owner->AddNode(this->attachment);
  attachmentAnim.Update(0, *this->attachment);

  // On shoot frame, drop projectile
  auto onFire = [this]() -> void {
    AUDIO.Play(AudioType::SPREADER);

    AirShot* airshot = new AirShot(GetOwner()->GetField(), GetOwner()->GetTeam(), damage);
    airshot->SetDirection(Direction::RIGHT);
    auto props = airshot->GetHitboxProperties();
    props.aggressor = GetOwnerAs<Character>();
    airshot->SetHitboxProperties(props);

    GetOwner()->GetField()->AddEntity(*airshot, GetOwner()->GetTile()->GetX() + 1, GetOwner()->GetTile()->GetY());
  };

  this->AddAction(2, onFire);
}

AirShotCardAction::~AirShotCardAction()
{
}

void AirShotCardAction::OnUpdate(float _elapsed)
{
  attachmentAnim.Update(_elapsed, *this->attachment);
  CardAction::OnUpdate(_elapsed);
}

void AirShotCardAction::EndAction()
{
  this->GetOwner()->RemoveNode(attachment);
  GetOwner()->FreeComponentByID(this->GetID());
  delete this;
}
