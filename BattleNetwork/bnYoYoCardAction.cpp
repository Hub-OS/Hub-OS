#include "bnYoYoCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnYoYo.h"

#define NODE_PATH "resources/spells/buster_yoyo.png"
#define NODE_ANIM "resources/spells/buster_yoyo.animation"

#define FRAME1 { 1, 0.05 }
#define FRAME3 { 1, 5.0 }

#define FRAMES FRAME1, FRAME3


YoYoCardAction::YoYoCardAction(Character * owner, int damage) : CardAction(owner, "PLAYER_SHOOTING", &attachment, "Buster"), attachmentAnim(NODE_ANIM) {
  this->damage = damage;

  this->attachment = new SpriteProxyNode();
  this->attachment->setTexture(TextureResourceManager::GetInstance().LoadTextureFromFile(NODE_PATH));
  this->attachment->SetLayer(-1);

  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("DEFAULT");

  // add override anims
  this->OverrideAnimationFrames({ FRAMES });

}

YoYoCardAction::~YoYoCardAction()
{
}

void YoYoCardAction::Execute() {
  auto owner = GetOwner();

  owner->AddNode(this->attachment);
  attachmentAnim.Update(0, this->attachment->getSprite());

  yoyo = nullptr;

  // On shoot frame, drop projectile
  auto onFire = [this]() -> void {
    AUDIO.Play(AudioType::TOSS_ITEM_LITE);

    YoYo* y = new YoYo(GetOwner()->GetField(), GetOwner()->GetTeam(), damage);
    y->SetDirection(Direction::RIGHT);
    auto props = y->GetHitboxProperties();
    props.aggressor = GetOwnerAs<Character>();
    y->SetHitboxProperties(props);
    yoyo = y;
    GetOwner()->GetField()->AddEntity(*y, GetOwner()->GetTile()->GetX() + 1, GetOwner()->GetTile()->GetY());
  };

  this->AddAction(1, onFire);
}

void YoYoCardAction::OnUpdate(float _elapsed)
{
  attachmentAnim.Update(_elapsed, this->attachment->getSprite());
  CardAction::OnUpdate(_elapsed);

  if (yoyo && yoyo->IsDeleted()) {
    yoyo = nullptr;

    GetOwner()->GetFirstComponent<AnimationComponent>()->SetAnimation("PLAYER_IDLE");
    this->EndAction();
  }
}

void YoYoCardAction::EndAction()
{
  if (yoyo) {
    yoyo->Delete();
  }

  this->GetOwner()->RemoveNode(attachment);
  GetOwner()->FreeComponentByID(this->GetID());
  this->RecallPreviousState();

  delete this;
}
