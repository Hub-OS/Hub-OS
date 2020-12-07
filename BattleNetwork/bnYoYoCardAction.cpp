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

YoYoCardAction::YoYoCardAction(Character * owner, int damage) :
  attachmentAnim(NODE_ANIM), yoyo(nullptr),
  CardAction(*owner, "PLAYER_SHOOTING") {
  YoYoCardAction::damage = damage;

  attachment = new SpriteProxyNode();
  attachment->setTexture(Textures().LoadTextureFromFile(NODE_PATH));
  attachment->SetLayer(-1);

  attachmentAnim = Animation(NODE_ANIM);
  attachmentAnim.SetAnimation("DEFAULT");

  Animation& userAnim = user.GetFirstComponent<AnimationComponent>()->GetAnimationObject();
  AddAttachment(userAnim, "BUSTER", *attachment).PrepareAnimation(attachmentAnim);

  // add override anims
  OverrideAnimationFrames({ FRAMES });

  AddAttachment(*owner, "buster", *attachment).UseAnimation(attachmentAnim);

}

YoYoCardAction::~YoYoCardAction()
{
}

void YoYoCardAction::OnExecute() {
  auto owner = GetOwner();

  // On shoot frame, drop projectile
  auto onFire = [this]() -> void {
    auto& user = GetUser();
    Audio().Play(AudioType::TOSS_ITEM_LITE);

    Team team = GetOwner()->GetTeam();
    YoYo* y = new YoYo(GetOwner()->GetField(), team, damage);

    y->SetDirection(team == Team::red? Direction::right : Direction::left);
    auto props = y->GetHitboxProperties();
    props.aggressor = &user;
    y->SetHitboxProperties(props);
    yoyo = y;
    GetUser().GetField()->AddEntity(*y, user.GetTile()->GetX() + 1, user.GetTile()->GetY());
  };

  AddAnimAction(1, onFire);
}

void YoYoCardAction::OnUpdate(float _elapsed)
{
  CardAction::OnUpdate(_elapsed);

  if (yoyo && yoyo->WillRemoveLater()) {
    yoyo = nullptr;

    RecallPreviousState();
    EndAction();
  }
}

void YoYoCardAction::OnAnimationEnd()
{
}

void YoYoCardAction::OnEndAction()
{
  if (yoyo) {
    yoyo->Delete();
  }
  Eject();
}
