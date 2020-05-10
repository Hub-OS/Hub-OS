#include "bnTwinFangCardAction.h"
#include "bnCardAction.h"
#include "bnSpriteProxyNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnTwinFang.h"

#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.05 }

#define FRAMES FRAME1, FRAME1, FRAME1, FRAME1, FRAME2, FRAME3, FRAME2, FRAME1, FRAME1


TwinFangCardAction::TwinFangCardAction(Character& user, int damage) : CardAction(user, "PLAYER_SHOOTING") {
  TwinFangCardAction::damage = damage;

  // add override anims
  OverrideAnimationFrames({ FRAMES });
}

void TwinFangCardAction::OnExecute() {
  // On shoot frame, drop projectile
  auto onFire = [this]() -> void {
    auto& user = GetUser();

    auto tile = GetUser().GetTile();

    AUDIO.Play(AudioType::TOSS_ITEM_LITE);

    if (tile->GetY() != 0) {
      TwinFang* twinfang = new TwinFang(user.GetField(), user.GetTeam(), TwinFang::Type::ABOVE, damage);
      auto props = twinfang->GetHitboxProperties();
      props.aggressor = &user;
      twinfang->SetHitboxProperties(props);
      twinfang->SetDirection(Direction::right);

      GetUser().GetField()->AddEntity(*twinfang, tile->GetX(), tile->GetY() - 1);
    }

    if (tile->GetY() != 4) {
      TwinFang* twinfang = new TwinFang(user.GetField(), user.GetTeam(), TwinFang::Type::BELOW, damage);
      auto props = twinfang->GetHitboxProperties();
      props.aggressor = &user;
      twinfang->SetHitboxProperties(props);
      twinfang->SetDirection(Direction::right);

      GetUser().GetField()->AddEntity(*twinfang, tile->GetX(), tile->GetY() + 1);
    }
  };

  AddAction(2, onFire);
}

void TwinFangCardAction::OnEndAction()
{
  // do nothing special
}

TwinFangCardAction::~TwinFangCardAction()
{
}
