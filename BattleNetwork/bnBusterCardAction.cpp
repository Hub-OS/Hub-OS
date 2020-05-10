#include "bnBusterCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnBuster.h"

#define NODE_PATH "resources/spells/buster_shoot.png"
#define NODE_ANIM "resources/spells/buster_shoot.animation"

BusterCardAction::BusterCardAction(Character& user, bool charged, int damage) : CardAction(user, "PLAYER_SHOOTING") {
  BusterCardAction::damage = damage;
  BusterCardAction::charged = charged;

  buster = new SpriteProxyNode();
  buster->setTexture(user.getTexture());
  buster->SetLayer(-1);
  buster->EnableParentShader();

  busterAnim = Animation(anim->GetFilePath());
  busterAnim.SetAnimation("BUSTER");

  flare = new SpriteProxyNode();
  flare->setTexture(TextureResourceManager::GetInstance().LoadTextureFromFile(NODE_PATH));
  flare->SetLayer(-1);

  flareAnim = Animation(NODE_ANIM);
  flareAnim.Reload();
  flareAnim.SetAnimation("DEFAULT");

  AddAttachment(anim->GetAnimationObject(), "buster", *buster).PrepareAnimation(busterAnim);
  AddAttachment(busterAnim, "endpoint", *flare).PrepareAnimation(flareAnim);
}

void BusterCardAction::OnExecute() {

  // On shoot frame, drop projectile
  auto onFire = [this]() -> void {
    auto& user = GetUser();

    Buster* b = new Buster(user.GetField(), user.GetTeam(), charged, damage);
    b->SetDirection(Direction::right);
    auto props = b->GetHitboxProperties();
    b->SetHitboxProperties(props);

    auto status = user.GetField()->AddEntity(*b, *user.GetTile());

    if (status != Field::AddEntityStatus::deleted) {
      Entity::RemoveCallback& busterRemoved = b->CreateRemoveCallback();
      busterRemoved.Slot([this]() {
        EndAction();
      });

      AUDIO.Play(AudioType::BUSTER_PEA);
    }
  };

  AddAction(1, onFire);
}

BusterCardAction::~BusterCardAction()
{
  delete buster;
  delete flare;
}
void BusterCardAction::OnEndAction()
{
    GetUser().RemoveNode(buster);
    GetUser().RemoveNode(flare);
}
