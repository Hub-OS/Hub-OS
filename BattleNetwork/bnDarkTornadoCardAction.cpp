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

DarkTornadoCardAction::DarkTornadoCardAction(Character& owner, int damage) 
: CardAction(owner, "PLAYER_SHOOTING"), 
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

void DarkTornadoCardAction::OnExecute() {
  auto& owner = GetCharacter();

  attachmentAnim.Update(0, attachment->getSprite());

  owner.AddNode(attachment);

  auto team = owner.GetTeam();
  auto tile = owner.GetTile();
  auto field = owner.GetField();

  // On shoot frame, drop projectile
  auto onFire = [this, team, tile, field, owner = &owner]() -> void {
    Tornado* tornado = new Tornado(team, 8, damage);
    tornado->setTexture(Textures().LoadTextureFromFile("resources/spells/spell_tornado_dark.png"));

    auto props = tornado->GetHitboxProperties();
    props.aggressor = owner;
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

void DarkTornadoCardAction::OnEndAction()
{
  GetCharacter().RemoveNode(attachment);
}
