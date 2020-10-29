#include "bnBusterCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnBuster.h"

#define NODE_PATH "resources/spells/buster_shoot.png"
#define NODE_ANIM "resources/spells/buster_shoot.animation"

BusterCardAction::BusterCardAction(Character * owner, bool charged, int damage) 
  : CardAction(*owner, "PLAYER_SHOOTING")
{
  BusterCardAction::damage = damage;
  BusterCardAction::charged = charged;

  buster = new SpriteProxyNode();
  buster->setTexture(owner->getTexture());
  buster->SetLayer(-1);

  busterAnim = Animation(owner->GetFirstComponent<AnimationComponent>()->GetFilePath());
  busterAnim.SetAnimation("BUSTER");

  flare = new SpriteProxyNode();
  flare->setTexture(TextureResourceManager::GetInstance().LoadTextureFromFile(NODE_PATH));
  flare->SetLayer(-1);

  flareAnim = Animation(NODE_ANIM);
  flareAnim.SetAnimation("DEFAULT");

  isBusterAlive = false;

  busterAttachment = &AddAttachment(*owner, "buster", *buster).UseAnimation(busterAnim);

  this->SetLockout(ActionLockoutProperties{ ActionLockoutType::async, 0.5 });
}

void BusterCardAction::Execute() {
  auto owner = GetOwner();

  buster->EnableParentShader(true);

  // On shoot frame, drop projectile
  auto onFire = [this]() -> void {
    Team team = this->GetOwner()->GetTeam();
    Buster* b = new Buster(GetOwner()->GetField(), team, charged, damage);

    if (team == Team::red) {
      b->SetDirection(Direction::right);
    }
    else {
      b->SetDirection(Direction::left);
    }

    auto props = b->GetHitboxProperties();
    b->SetHitboxProperties(props);

    isBusterAlive = true;
    Entity::RemoveCallback& busterRemoved = b->CreateRemoveCallback();
    busterRemoved.Slot([this]() {
      isBusterAlive = false;
      EndAction();
    });

    GetOwner()->GetField()->AddEntity(*b, *GetOwner()->GetTile());
    AUDIO.Play(AudioType::BUSTER_PEA);

    busterAttachment->AddAttachment(busterAnim, "endpoint", *flare).UseAnimation(flareAnim);
  };

  AddAnimAction(3, onFire);
}

BusterCardAction::~BusterCardAction()
{
  delete buster;
  delete flare;
}

void BusterCardAction::OnUpdate(float _elapsed)
{
  CardAction::OnUpdate(_elapsed);
}

void BusterCardAction::OnAnimationEnd()
{
  isBusterAlive = false;
}

void BusterCardAction::EndAction()
{
  OnAnimationEnd();
  Eject();
}
