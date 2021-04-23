#include "bnBusterCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnBuster.h"
#include "bnField.h"

#define NODE_PATH "resources/spells/buster_shoot.png"
#define NODE_ANIM "resources/spells/buster_shoot.animation"

BusterCardAction::BusterCardAction(Character& actor, bool charged, int damage) : CardAction(actor, "PLAYER_SHOOTING")
{
  BusterCardAction::damage = damage;
  BusterCardAction::charged = charged;

  buster = new SpriteProxyNode();
  buster->setTexture(actor.getTexture());
  buster->SetLayer(-1);

  busterAnim = Animation(actor.GetFirstComponent<AnimationComponent>()->GetFilePath());
  busterAnim.SetAnimation("BUSTER");

  flare = new SpriteProxyNode();
  flare->setTexture(Textures().LoadTextureFromFile(NODE_PATH));
  flare->SetLayer(-1);

  flareAnim = Animation(NODE_ANIM);
  flareAnim.SetAnimation("DEFAULT");

  busterAttachment = &AddAttachment(actor, "buster", *buster).UseAnimation(busterAnim);

  this->SetLockout({ CardAction::LockoutType::async, 0.5 });
}

void BusterCardAction::OnExecute(Character* user) {
  buster->EnableParentShader(true);

  // On shoot frame, drop projectile
  auto onFire = [this, user]() -> void {
    Team team = user->GetTeam();
    Buster* b = new Buster(team, charged, damage);

    if (team == Team::red) {
      b->SetDirection(Direction::right);
    }
    else {
      b->SetDirection(Direction::left);
    }

    busterRemoved = &b->CreateRemoveCallback();
    busterRemoved->Slot([this](Entity*) {
      EndAction();
      });

    user->GetField()->AddEntity(*b, *user->GetTile());
    Audio().Play(AudioType::BUSTER_PEA);

    busterAttachment->AddAttachment(busterAnim, "endpoint", *flare).UseAnimation(flareAnim);
  };

  AddAnimAction(2, onFire);
}

BusterCardAction::~BusterCardAction()
{
  if (busterRemoved) {
    busterRemoved->Reset();
  }
}

void BusterCardAction::Update(double _elapsed)
{
  CardAction::Update(_elapsed);
}

void BusterCardAction::OnActionEnd()
{
}

void BusterCardAction::OnAnimationEnd()
{
}