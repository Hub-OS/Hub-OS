#include "bnAirShotChipAction.h"
#include "bnChipAction.h"
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


AirShotChipAction::AirShotChipAction(Character * owner, int damage) : ChipAction(owner, "PLAYER_SHOOTING", &attachment, "Buster"), attachmentAnim(NODE_ANIM) {
  this->damage = damage;
}

void AirShotChipAction::Execute() {
  auto owner = GetOwner();
  airshot.setTexture(*TextureResourceManager::GetInstance().LoadTextureFromFile(NODE_PATH));
  this->attachment = new SpriteSceneNode(airshot);
  this->attachment->SetLayer(-1);
  owner->AddNode(this->attachment);

  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("DEFAULT");
  attachmentAnim.Update(0, *this->attachment);

  // add override anims
  this->OverrideAnimationFrames({ FRAMES });

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

AirShotChipAction::~AirShotChipAction()
{
}

void AirShotChipAction::OnUpdate(float _elapsed)
{
  attachmentAnim.Update(_elapsed, *this->attachment);
  ChipAction::OnUpdate(_elapsed);
}

void AirShotChipAction::EndAction()
{
  this->GetOwner()->RemoveNode(attachment);
  GetOwner()->FreeComponentByID(this->GetID());
  delete this;
}
