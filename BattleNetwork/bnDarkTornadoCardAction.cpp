#include "bnDarkTornadoCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnTornado.h"
#include "bnField.h"

#define FAN_PATH "resources/spells/buster_fan_dark.png"
#define FAN_ANIM "resources/spells/buster_fan_dark.animation"

#define FRAME1 { 1, 0.1  }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.05 }

#define FRAMES FRAME1, FRAME3, FRAME2, FRAME3, FRAME2, \
        FRAME3, FRAME2, FRAME3, FRAME2, FRAME3, FRAME2, \
        FRAME3, FRAME2, FRAME3, FRAME2, FRAME3, FRAME2, FRAME3

DarkTornadoCardAction::DarkTornadoCardAction(Character* actor, int damage) 
: CardAction(actor, "PLAYER_SHOOTING"), 
  attachmentAnim(FAN_ANIM), armIsOut(false), damage(damage) {
  fan.setTexture(*Textures().LoadTextureFromFile(FAN_PATH));
  attachment = new SpriteProxyNode(fan);
  attachment->SetLayer(-1);

  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("DEFAULT");
  attachmentAnim << Animator::Mode::Loop;

  // add override anims
  OverrideAnimationFrames({ FRAMES });
}

DarkTornadoCardAction::~DarkTornadoCardAction()
{
}

void DarkTornadoCardAction::OnExecute(Character* user) {
  auto* actor = this->GetActor();

  attachmentAnim.Update(0, attachment->getSprite());

  actor->AddNode(attachment);

  auto team = actor->GetTeam();
  auto tile = actor->GetTile();
  auto field = actor->GetField();

  // On shoot frame, drop projectile
  auto onFire = [=]() -> void {
    Tornado* tornado = new Tornado(team, 8, damage);
    tornado->setTexture(Textures().LoadTextureFromFile("resources/spells/spell_tornado_dark.png"));

    auto props = tornado->GetHitboxProperties();
    props.aggressor = user->GetID();
    tornado->SetHitboxProperties(props);

    int step = team == Team::red ? 2 : -2;
    field->AddEntity(*tornado, tile->GetX() + step, tile->GetY());
  };

  // Spawn a tornado istance 2 tiles in front of the player every x frames 8 times
  AddAnimAction(2, [onFire, this]() {
    Audio().Play(AudioType::WIND);
    armIsOut = true;
    onFire();
  });
}

void DarkTornadoCardAction::Update(double _elapsed)
{
  attachment->setPosition(CalculatePointOffset("buster"));

  // manually update anim if attached to arm
  if (armIsOut) {
    attachmentAnim.Update(_elapsed, attachment->getSprite());
  }

  CardAction::Update(_elapsed);
}

void DarkTornadoCardAction::OnAnimationEnd()
{
}

void DarkTornadoCardAction::OnActionEnd()
{
  GetActor()->RemoveNode(attachment);
}
