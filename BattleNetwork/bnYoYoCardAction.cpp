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


YoYoCardAction::YoYoCardAction(Character * user, int damage) : CardAction(user, "PLAYER_SHOOTING", &attachment, "Buster"), attachmentAnim(NODE_ANIM) {
  YoYoCardAction::damage = damage;

  attachment = new SpriteProxyNode();
  attachment->setTexture(TextureResourceManager::GetInstance().LoadTextureFromFile(NODE_PATH));
  attachment->SetLayer(-1);

  attachmentAnim.Reload();
  attachmentAnim.SetAnimation("DEFAULT");

  // add override anims
  OverrideAnimationFrames({ FRAMES });

}

YoYoCardAction::~YoYoCardAction()
{
}

void YoYoCardAction::Execute() {
  auto user = GetUser();

  user->AddNode(attachment);
  attachmentAnim.Update(0, attachment->getSprite());

  yoyo = nullptr;

  // On shoot frame, drop projectile
  auto onFire = [this, user]() -> void {
    AUDIO.Play(AudioType::TOSS_ITEM_LITE);

    YoYo* y = new YoYo(GetUser()->GetField(), GetUser()->GetTeam(), damage);
    y->SetDirection(Direction::right);
    auto props = y->GetHitboxProperties();
    props.aggressor = user;
    y->SetHitboxProperties(props);
    yoyo = y;
    GetUser()->GetField()->AddEntity(*y, GetUser()->GetTile()->GetX() + 1, GetUser()->GetTile()->GetY());
  };

  AddAction(1, onFire);
}

void YoYoCardAction::OnUpdate(float _elapsed)
{
  attachmentAnim.Update(_elapsed, attachment->getSprite());
  CardAction::OnUpdate(_elapsed);

  if (yoyo && yoyo->IsDeleted()) {
    yoyo = nullptr;

    GetUser()->GetFirstComponent<AnimationComponent>()->SetAnimation("PLAYER_IDLE");
    EndAction();
  }
}

void YoYoCardAction::EndAction()
{
  if (yoyo) {
    yoyo->Delete();
  }

  GetUser()->RemoveNode(attachment);
  GetUser()->EndCurrentAction();
}
