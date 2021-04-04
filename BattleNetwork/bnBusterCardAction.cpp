#include "bnBusterCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnBuster.h"
#include "bnField.h"

#define NODE_PATH "resources/spells/buster_shoot.png"
#define NODE_ANIM "resources/spells/buster_shoot.animation"

BusterCardAction::BusterCardAction(Character& owner, bool charged, int damage) 
  : CardAction(owner, "PLAYER_SHOOTING")
{
  BusterCardAction::damage = damage;
  BusterCardAction::charged = charged;

  buster = new SpriteProxyNode();
  buster->setTexture(owner.getTexture());
  buster->SetLayer(-1);

  busterAnim = Animation(owner.GetFirstComponent<AnimationComponent>()->GetFilePath());
  busterAnim.SetAnimation("BUSTER");

  flare = new SpriteProxyNode();
  flare->setTexture(Textures().LoadTextureFromFile(NODE_PATH));
  flare->SetLayer(-1);

  flareAnim = Animation(NODE_ANIM);
  flareAnim.SetAnimation("DEFAULT");

  isBusterAlive = false;

  busterAttachment = &AddAttachment(owner, "buster", *buster).UseAnimation(busterAnim);

  this->SetLockout({ CardAction::LockoutType::async, 0.5 });
}

void BusterCardAction::OnExecute() {
  auto owner = &GetCharacter();

  buster->EnableParentShader(true);

  // On shoot frame, drop projectile
  auto onFire = [this, owner]() -> void {
    Team team = owner->GetTeam();
    Buster* b = new Buster(team, charged, damage);

    if (team == Team::red) {
      b->SetDirection(Direction::right);
    }
    else {
      b->SetDirection(Direction::left);
    }

    auto props = b->GetHitboxProperties();
    b->SetHitboxProperties(props);

    isBusterAlive = true;
    busterRemoved = &b->CreateRemoveCallback();
    busterRemoved->Slot([this](Entity*) {
      isBusterAlive = false;
      EndAction();
      });

    owner->GetField()->AddEntity(*b, *owner->GetTile());
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

void BusterCardAction::OnEndAction()
{
  OnAnimationEnd();
}

void BusterCardAction::OnAnimationEnd()
{
  isBusterAlive = false;
}