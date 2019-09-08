#include "bnTornadoChipAction.h"
#include "bnChipAction.h"
#include "bnSpriteSceneNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnTornado.h"

#define FAN_PATH "resources/spells/buster_fan.png"
#define FAN_ANIM "resources/spells/buster_fan.animation"

#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.05 }
#define FRAME4 { 4, 0.05 }
#define FRAME5 { 5, 0.05 }

#define FRAMES FRAME4, FRAME5, FRAME4, FRAME5, FRAME4, FRAME5, FRAME4, FRAME5, FRAME4, FRAME5, FRAME4, FRAME5, FRAME4, FRAME5, FRAME4, FRAME5


TornadoChipAction::TornadoChipAction(Character * owner, int damage) : ChipAction(owner, "PLAYER_SHOOTING", &attachment, "Buster"), attachmentAnim(FAN_ANIM) {
  fan.setTexture(*TextureResourceManager::GetInstance().LoadTextureFromFile(FAN_PATH));
  this->attachment = new SpriteSceneNode(fan);
  this->attachment->SetLayer(-1);
  owner->AddNode(this->attachment);

  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("DEFAULT");
  attachmentAnim.Update(0, *this->attachment);

  // add override anims
  this->OverrideAnimationFrames({ FRAMES });

  // On shoot frame, drop projectile
  auto onFire = [this, damage, owner]() -> void {
    Tornado* tornado = new Tornado(GetOwner()->GetField(), GetOwner()->GetTeam(), damage);
    auto props = tornado->GetHitboxProperties();
    props.aggressor = GetOwnerAs<Character>();
    tornado->SetHitboxProperties(props);

    // AUDIO.Play(AudioType::FAN);

    GetOwner()->GetField()->AddEntity(*tornado, GetOwner()->GetTile()->GetX() + 2, GetOwner()->GetTile()->GetY());
  };

  // Spawn a tornado istance 2 tiles in front of the player every x frames 8 times
  this->AddAction(1, onFire);
  this->AddAction(3, onFire);
  this->AddAction(5, onFire);
  this->AddAction(7, onFire);
  this->AddAction(9, onFire);
  this->AddAction(11, onFire);
  this->AddAction(13, onFire);
  this->AddAction(15, onFire);
}

TornadoChipAction::~TornadoChipAction()
{
}

void TornadoChipAction::OnUpdate(float _elapsed)
{
  attachmentAnim.Update(_elapsed, *this->attachment);
  ChipAction::OnUpdate(_elapsed);
}

void TornadoChipAction::EndAction()
{
  this->GetOwner()->RemoveNode(attachment);
  GetOwner()->FreeComponentByID(this->GetID());
  delete this;
}
