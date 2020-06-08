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


TornadoCardAction::TornadoCardAction(Character& user, int damage) 
  : CardAction(user, "PLAYER_SHOOTING"), armIsOut(false) {
  TornadoCardAction::damage = damage;

  attachment = new SpriteProxyNode();
  attachment->setTexture(Textures().LoadTextureFromFile(FAN_PATH));
  attachment->SetLayer(-1);

  attachmentAnim = Animation(FAN_PATH);
  attachmentAnim.SetAnimation("DEFAULT");
  attachmentAnim << Animator::Mode::Loop;

  Animation& userAnim = user.GetFirstComponent<AnimationComponent>()->GetAnimationObject();
  AddAttachment(userAnim, "BUSTER", *attachment).PrepareAnimation(attachmentAnim);

  // add override anims
  OverrideAnimationFrames({ FRAMES });
}

TornadoCardAction::~TornadoCardAction()
{
}

void TornadoCardAction::OnExecute() {
  // On shoot frame, drop projectile
  auto onFire = [this]() -> void {
    auto& user = GetUser();
    auto team = user.GetTeam();
    auto tile = user.GetTile();
    auto field = user.GetField();

    Tornado* tornado = new Tornado(field, team, damage);
    auto props = tornado->GetHitboxProperties();
    props.aggressor = &user;
    tornado->SetHitboxProperties(props);

    field->AddEntity(*tornado, tile->GetX() + 2, tile->GetY());
  };

  // Spawn a tornado istance 2 tiles in front of the player every x frames 8 times
  AddAction(2, [onFire, this]() {
    Audio().Play(AudioType::WIND);
    armIsOut = true;
    onFire();
  });

  AddAction(4, onFire);
  AddAction(6, onFire);
  AddAction(8, onFire);
  AddAction(10, onFire);
  AddAction(12, onFire);
  AddAction(14, onFire);
  AddAction(16, onFire);
}

void TornadoCardAction::OnUpdate(float _elapsed)
{
  if (armIsOut) {
    attachmentAnim.Update(_elapsed, attachment->getSprite());
  }

  CardAction::OnUpdate(_elapsed);
}

void TornadoCardAction::OnEndAction()
{
  GetUser().RemoveNode(attachment);
}
