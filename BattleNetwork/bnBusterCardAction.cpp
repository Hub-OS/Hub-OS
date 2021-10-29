#include "bnBusterCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnBuster.h"
#include "bnPlayer.h"
#include "bnField.h"

#define NODE_PATH "resources/scenes/battle/spells/buster_shoot.png"
#define NODE_ANIM "resources/scenes/battle/spells/buster_shoot.animation"

BusterCardAction::BusterCardAction(std::weak_ptr<Character> actorWeak, bool charged, int damage) : CardAction(actorWeak, "PLAYER_SHOOTING")
{
  BusterCardAction::damage = damage;
  BusterCardAction::charged = charged;

  auto actor = actorWeak.lock();

  busterAttachment = &AddAttachment("buster");

  auto& busterAnim = busterAttachment->GetAnimationObject();
  busterAnim.Load(actor->GetFirstComponent<AnimationComponent>()->GetFilePath());
  busterAnim.SetAnimation("BUSTER");

  buster = busterAttachment->GetSpriteNode();
  buster->setTexture(actor->getTexture());
  buster->SetLayer(-1);
  buster->EnableParentShader(true);
  buster->AddTags({Player::BASE_NODE_TAG});

  this->SetLockout({ CardAction::LockoutType::async, 0.5 });
}

void BusterCardAction::OnExecute(std::shared_ptr<Character> user) {
  buster->setColor(user->getColor());

  // On shoot frame, drop projectile
  auto onFire = [this, user]() -> void {
    Team team = user->GetTeam();
    auto b = std::make_shared<Buster>(team, charged, damage);
    auto field = user->GetField();

    if (team == Team::red) {
      b->SetMoveDirection(Direction::right);
    }
    else {
      b->SetMoveDirection(Direction::left);
    }

    auto busterRemoved = [this](auto target) {
      EndAction();
    };

    notifier = field->CallbackOnDelete(b->GetID(), busterRemoved);

    field->AddEntity(b, *user->GetTile());
    Audio().Play(AudioType::BUSTER_PEA);

    auto& attachment = busterAttachment->AddAttachment("endpoint");

    auto flare = attachment.GetSpriteNode();
    flare->setTexture(Textures().LoadFromFile(NODE_PATH));
    flare->SetLayer(-1);

    auto& flareAnim = attachment.GetAnimationObject();
    flareAnim = Animation(NODE_ANIM);
    flareAnim.SetAnimation("DEFAULT");
  };

  AddAnimAction(2, onFire);
}

void BusterCardAction::Update(double _elapsed)
{
  CardAction::Update(_elapsed);
}

void BusterCardAction::OnActionEnd()
{
  auto actor = GetActor();
  if (!actor) return;

  auto field = actor->GetField();

  if (field) {
    field->DropNotifier(this->notifier);
  }
}

void BusterCardAction::OnAnimationEnd()
{
}