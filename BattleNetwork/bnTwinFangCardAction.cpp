#include "bnTwinFangCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnTwinFang.h"
#include "bnField.h"

#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.05 }

#define FRAMES FRAME1, FRAME1, FRAME1, FRAME1, FRAME2, FRAME3, FRAME2, FRAME1, FRAME1

TwinFangCardAction::TwinFangCardAction(Character* actor, int damage) : 
  CardAction(actor, "PLAYER_SHOOTING") {
  TwinFangCardAction::damage = damage;

  buster = new SpriteProxyNode();
  buster->setTexture(actor->getTexture());
  buster->SetLayer(-1);
  buster->EnableParentShader(true);

  busterAnim = Animation(actor->GetFirstComponent<AnimationComponent>()->GetFilePath());

  std::string newAnimState;
  busterAnim.OverrideAnimationFrames("BUSTER", { FRAMES }, newAnimState);
  busterAnim.SetAnimation(newAnimState);
  busterAttachment = &AddAttachment(actor, "buster", *buster).UseAnimation(busterAnim);

  // add override anims
  OverrideAnimationFrames({ FRAMES });
}

TwinFangCardAction::~TwinFangCardAction()
{
}

void TwinFangCardAction::OnExecute(Character* user) {
  // On shoot frame, drop projectile
  auto onFire = [=]() -> void {
    auto tiles = std::vector{
      user->GetTile()->Offset(0,-1),
      user->GetTile()->Offset(0, 1)
    };

    Direction dir = Direction::right;
    if (user->GetTeam() == Team::blue) {
      dir = Direction::left;
    }

    Audio().Play(AudioType::TOSS_ITEM_LITE);

    if (user->GetTile()->GetY() != 0) {
      TwinFang* twinfang = new TwinFang(user->GetTeam(), TwinFang::Type::ABOVE, damage);
      auto props = twinfang->GetHitboxProperties();
      props.aggressor = user->GetID();
      twinfang->SetHitboxProperties(props);
      twinfang->SetDirection(dir);

      user->GetField()->AddEntity(*twinfang, *tiles[0]);
    }

    if (user->GetTile()->GetY() != 4) {
      TwinFang* twinfang = new TwinFang(user->GetTeam(), TwinFang::Type::BELOW, damage);
      auto props = twinfang->GetHitboxProperties();
      props.aggressor = user->GetID();
      twinfang->SetHitboxProperties(props);
      twinfang->SetDirection(dir);

      user->GetField()->AddEntity(*twinfang, *tiles[1]);
    }
  };

  AddAnimAction(2, onFire);
}

void TwinFangCardAction::Update(double _elapsed)
{
  CardAction::Update(_elapsed);
}

void TwinFangCardAction::OnAnimationEnd()
{
}

void TwinFangCardAction::OnActionEnd()
{
}
