#include "bnTornadoCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnTornado.h"

#define FAN_PATH "resources/spells/buster_fan.png"
#define FAN_ANIM "resources/spells/buster_fan.animation"

#define FRAME1 { 1, 0.1  }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.05 }

#define FRAMES FRAME1, FRAME3, FRAME2, FRAME3, FRAME2, FRAME3, FRAME2, FRAME3, FRAME2, FRAME3, FRAME2, FRAME3, FRAME2, FRAME3, FRAME2, FRAME3, FRAME2, FRAME3


TornadoCardAction::TornadoCardAction(Character * owner, int damage) 
  : CardAction(owner, "PLAYER_SHOOTING", &attachment, "Buster"), attachmentAnim(FAN_ANIM), armIsOut(false) {
  this->damage = damage;
  fan.setTexture(*TextureResourceManager::GetInstance().LoadTextureFromFile(FAN_PATH));
  this->attachment = new SpriteProxyNode(fan);
  this->attachment->SetLayer(-1);

  owner->AddNode(this->attachment);

  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("DEFAULT");
  attachmentAnim << Animator::Mode::Loop;

  // add override anims
  this->OverrideAnimationFrames({ FRAMES });
}

TornadoCardAction::~TornadoCardAction()
{
}

void TornadoCardAction::Execute() {
  auto owner = GetOwner();

  attachmentAnim.Update(0, this->attachment->getSprite());

  auto team = GetOwner()->GetTeam();
  auto tile = GetOwner()->GetTile();
  auto field = GetOwner()->GetField();

  // On shoot frame, drop projectile
  auto onFire = [this, team, tile, field]() -> void {
    Tornado* tornado = new Tornado(field, team, damage);
    auto props = tornado->GetHitboxProperties();
    props.aggressor = GetOwnerAs<Character>();
    tornado->SetHitboxProperties(props);

    field->AddEntity(*tornado, tile->GetX() + 2, tile->GetY());
  };

  // Spawn a tornado istance 2 tiles in front of the player every x frames 8 times
  this->AddAction(2, [onFire, this]() {
    AUDIO.Play(AudioType::WIND);
    armIsOut = true;
    onFire();
  });

  this->AddAction(4, onFire);
  this->AddAction(6, onFire);
  this->AddAction(8, onFire);
  this->AddAction(10, onFire);
  this->AddAction(12, onFire);
  this->AddAction(14, onFire);
  this->AddAction(16, onFire);
}

void TornadoCardAction::OnUpdate(float _elapsed)
{
  if (armIsOut) {
    attachmentAnim.Update(_elapsed, this->attachment->getSprite());
  }

  CardAction::OnUpdate(_elapsed);
}

void TornadoCardAction::EndAction()
{
  this->GetOwner()->RemoveNode(attachment);
  GetOwner()->FreeComponentByID(this->GetID());
  delete this;
}
