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


YoYoCardAction::YoYoCardAction(Character * owner, int damage) 
  : attachmentAnim(NODE_ANIM), yoyo(nullptr),
  CardAction(owner, "PLAYER_SHOOTING", &attachment, "Buster") {
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
  auto owner = GetOwner();

  owner->AddNode(attachment);
  attachmentAnim.Update(0, attachment->getSprite());

  // On shoot frame, drop projectile
  auto onFire = [this]() -> void {
    AUDIO.Play(AudioType::TOSS_ITEM_LITE);

    Team team = GetOwner()->GetTeam();
    YoYo* y = new YoYo(GetOwner()->GetField(), team, damage);

    y->SetDirection(team == Team::red? Direction::right : Direction::left);

    auto props = y->GetHitboxProperties();
    props.aggressor = GetOwnerAs<Character>();
    y->SetHitboxProperties(props);
    yoyo = y;
    GetOwner()->GetField()->AddEntity(*y, GetOwner()->GetTile()->GetX() + 1, GetOwner()->GetTile()->GetY());
  };

  AddAnimAction(1, onFire);
}

void YoYoCardAction::OnUpdate(float _elapsed)
{
  attachmentAnim.Update(_elapsed, attachment->getSprite());
  CardAction::OnUpdate(_elapsed);

  if (yoyo && yoyo->WillRemoveLater()) {
    yoyo = nullptr;

    GetOwner()->GetFirstComponent<AnimationComponent>()->SetAnimation("PLAYER_IDLE");
    EndAction();
  }
}

void YoYoCardAction::OnAnimationEnd()
{
}

void YoYoCardAction::EndAction()
{
  if (yoyo) {
    yoyo->Delete();
  }

  GetOwner()->RemoveNode(attachment);
  RecallPreviousState();
  Eject();
}
