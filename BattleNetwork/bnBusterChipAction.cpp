#include "bnBusterChipAction.h"
#include "bnChipAction.h"
#include "bnSpriteSceneNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnBuster.h"

#define NODE_PATH "resources/spells/buster_shoot.png"
#define NODE_ANIM "resources/spells/buster_shoot.animation"

BusterChipAction::BusterChipAction(Character * owner, bool charged, int damage) : ChipAction(owner, "PLAYER_SHOOTING", &attachment2, "Buster"), 
attachmentAnim(owner->GetFirstComponent<AnimationComponent>()->GetFilePath()) {
  this->damage = damage;
  this->charged = charged;

  this->attachment2 = new SpriteSceneNode();
  this->attachment2->setTexture(*owner->getTexture());
  this->attachment2->SetLayer(-1);

  attachmentAnim2 = Animation(owner->GetFirstComponent<AnimationComponent>()->GetFilePath());
  attachmentAnim2.Reload();
  attachmentAnim2.SetAnimation("BUSTER");

  this->attachment = new SpriteSceneNode();
  this->attachment->setTexture(*TextureResourceManager::GetInstance().LoadTextureFromFile(NODE_PATH));
  this->attachment->SetLayer(-1);

  attachmentAnim = Animation(NODE_ANIM);
  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("DEFAULT");
}

void BusterChipAction::Execute() {
  auto owner = GetOwner();

  owner->AddNode(this->attachment2);
  attachment2->AddNode(this->attachment);


  attachmentAnim.Update(0, *this->attachment);

  this->attachment2->EnableParentShader(true);
  attachmentAnim2.Update(0, *this->attachment2);

  // On shoot frame, drop projectile
  auto onFire = [this]() -> void {
    Buster* b = new Buster(GetOwner()->GetField(), GetOwner()->GetTeam(), charged, damage);
    b->SetDirection(Direction::RIGHT);
    auto props = b->GetHitboxProperties();
    b->SetHitboxProperties(props);
    GetOwner()->GetField()->AddEntity(*b, *GetOwner()->GetTile());
    AUDIO.Play(AudioType::BUSTER_PEA);
  };

  this->AddAction(1, onFire);
}

BusterChipAction::~BusterChipAction()
{
  if (attachment) {
    delete attachment;
  }

  if (attachment2) {
    delete attachment2;
  }
}

void BusterChipAction::OnUpdate(float _elapsed)
{
  attachmentAnim2.Update(_elapsed, *this->attachment2);
  attachmentAnim.Update(_elapsed, *this->attachment);

  ChipAction::OnUpdate(_elapsed);

  // update node position in the animation
  auto baseOffset = attachmentAnim2.GetPoint("endpoint");
  auto origin = attachment2->getOrigin();
  baseOffset = baseOffset - origin;

  attachment->setPosition(baseOffset);
}

void BusterChipAction::EndAction()
{
  this->GetOwner()->RemoveNode(attachment2);
  attachment2->RemoveNode(attachment);
  GetOwner()->FreeComponentByID(this->GetID());

  delete this;
}
