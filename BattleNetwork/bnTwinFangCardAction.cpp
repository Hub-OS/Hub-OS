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

TwinFangCardAction::TwinFangCardAction(Character& owner, int damage) : 
  CardAction(owner, "PLAYER_SHOOTING") {
  TwinFangCardAction::damage = damage;

  buster = new SpriteProxyNode();
  buster->setTexture(owner.getTexture());
  buster->SetLayer(-1);
  buster->EnableParentShader(true);

  busterAnim = Animation(owner.GetFirstComponent<AnimationComponent>()->GetFilePath());

  std::string newAnimState;
  busterAnim.OverrideAnimationFrames("BUSTER", { FRAMES }, newAnimState);
  busterAnim.SetAnimation(newAnimState);
  busterAttachment = &AddAttachment(owner, "buster", *buster).UseAnimation(busterAnim);

  // add override anims
  OverrideAnimationFrames({ FRAMES });
}

TwinFangCardAction::~TwinFangCardAction()
{
}

void TwinFangCardAction::OnExecute() {
  // On shoot frame, drop projectile
  auto onFire = [this]() -> void {
    auto& user = GetCharacter();

    auto tiles = std::vector{
      user.GetTile()->Offset(0,-1),
      user.GetTile()->Offset(0, 1)
    };

    Audio().Play(AudioType::TOSS_ITEM_LITE);

    if (user.GetTile()->GetY() != 0) {
      TwinFang* twinfang = new TwinFang(user.GetTeam(), TwinFang::Type::ABOVE, damage);
      auto props = twinfang->GetHitboxProperties();
      props.aggressor = &user;
      twinfang->SetHitboxProperties(props);
      twinfang->SetDirection(Direction::right);

      user.GetField()->AddEntity(*twinfang, *tiles[0]);
    }

    if (user.GetTile()->GetY() != 4) {
      TwinFang* twinfang = new TwinFang(user.GetTeam(), TwinFang::Type::BELOW, damage);
      auto props = twinfang->GetHitboxProperties();
      props.aggressor = &user;
      twinfang->SetHitboxProperties(props);
      twinfang->SetDirection(Direction::right);

      user.GetField()->AddEntity(*twinfang, *tiles[1]);
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

void TwinFangCardAction::OnEndAction()
{
}
