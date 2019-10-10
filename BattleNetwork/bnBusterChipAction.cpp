#include "bnBusterChipAction.h"
#include "bnChipAction.h"
#include "bnSpriteSceneNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnBuster.h"

#define NODE_PATH "resources/spells/buster_shoot.png"
#define NODE_ANIM "resources/spells/buster_shoot.animation"

BusterChipAction::BusterChipAction(Character * owner, bool charged, int damage) : ChipAction(owner, "PLAYER_SHOOTING", &attachment, "Buster"), attachmentAnim(NODE_ANIM) {
  this->attachment = new SpriteSceneNode();
  this->attachment->setTexture(*TextureResourceManager::GetInstance().LoadTextureFromFile(NODE_PATH));
  this->attachment->SetLayer(-1);
  owner->AddNode(this->attachment);

  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("DEFAULT");
  attachmentAnim.Update(0, *this->attachment);

  // On shoot frame, drop projectile
  auto onFire = [this, damage, charged]() -> void {
    Buster* b = new Buster(GetOwner()->GetField(), GetOwner()->GetTeam(), charged, damage);
    b->SetDirection(Direction::RIGHT);
    auto props = b->GetHitboxProperties();
    props.aggressor = GetOwnerAs<Character>();
    b->SetHitboxProperties(props);
    GetOwner()->GetField()->AddEntity(*b, *GetOwner()->GetTile());
    AUDIO.Play(AudioType::BUSTER_PEA);
  };

  this->AddAction(1, onFire);
}

BusterChipAction::~BusterChipAction()
{
  delete attachment;
}

void BusterChipAction::OnUpdate(float _elapsed)
{
  attachmentAnim.Update(_elapsed, *this->attachment);
  ChipAction::OnUpdate(_elapsed);
}

void BusterChipAction::EndAction()
{
  this->GetOwner()->RemoveNode(attachment);
  GetOwner()->FreeComponentByID(this->GetID());

  delete this;
}
