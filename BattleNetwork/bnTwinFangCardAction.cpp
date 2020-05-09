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


TwinFangCardAction::TwinFangCardAction(Character * user, int damage) : CardAction(user, "PLAYER_SHOOTING", nullptr, "Buster") {
  TwinFangCardAction::damage = damage;

  // add override anims
  OverrideAnimationFrames({ FRAMES });
}

void TwinFangCardAction::Execute() {
  auto user = GetUser();

  // On shoot frame, drop projectile
  auto onFire = [this, user]() -> void {
    auto tile = GetUser()->GetTile();

    AUDIO.Play(AudioType::TOSS_ITEM_LITE);

    /**
    
    These duds were written before the grid was surrounded by invisible tiles
    The original games have invisible tiles for items that appear to float or go off the stage
    This can now be re-written to not need twin fang dud objects...
    */
    if (tile->GetY() != 1) {
      TwinFang* twinfang = new TwinFang(GetUser()->GetField(), GetUser()->GetTeam(), TwinFang::Type::ABOVE, damage);
      auto props = twinfang->GetHitboxProperties();
      props.aggressor = user;
      twinfang->SetHitboxProperties(props);
      twinfang->SetDirection(Direction::right);

      GetUser()->GetField()->AddEntity(*twinfang, tile->GetX(), tile->GetY() - 1);
    }
    else { // TwinFang floats above the scene
      TwinFang* twinfang = new TwinFang(GetUser()->GetField(), GetUser()->GetTeam(), TwinFang::Type::ABOVE_DUD, damage);
      auto props = twinfang->GetHitboxProperties();
      props.aggressor = user;
      twinfang->SetHitboxProperties(props);
      twinfang->SetDirection(Direction::right);

      GetUser()->GetField()->AddEntity(*twinfang, tile->GetX(), tile->GetY());
    }

    if (tile->GetY() != 3) {
      TwinFang* twinfang = new TwinFang(GetUser()->GetField(), GetUser()->GetTeam(), TwinFang::Type::BELOW, damage);
      auto props = twinfang->GetHitboxProperties();
      props.aggressor = user;
      twinfang->SetHitboxProperties(props);
      twinfang->SetDirection(Direction::right);

      GetUser()->GetField()->AddEntity(*twinfang, tile->GetX(), tile->GetY() + 1);
    }
    else { // TwinFang floats below the scene
      TwinFang* twinfang = new TwinFang(GetUser()->GetField(), GetUser()->GetTeam(), TwinFang::Type::BELOW_DUD, damage);
      auto props = twinfang->GetHitboxProperties();
      props.aggressor = user;
      twinfang->SetHitboxProperties(props);
      twinfang->SetDirection(Direction::right);

      GetUser()->GetField()->AddEntity(*twinfang, tile->GetX(), tile->GetY());
    }
  };

  AddAction(2, onFire);
}

TwinFangCardAction::~TwinFangCardAction()
{
}

void TwinFangCardAction::OnUpdate(float _elapsed)
{
  CardAction::OnUpdate(_elapsed);
}

void TwinFangCardAction::EndAction()
{
  GetUser()->EndCurrentAction();
}
