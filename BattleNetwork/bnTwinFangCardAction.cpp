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


TwinFangCardAction::TwinFangCardAction(Character * owner, int damage) : CardAction(owner, "PLAYER_SHOOTING", nullptr, "Buster") {
  TwinFangCardAction::damage = damage;

  // add override anims
  OverrideAnimationFrames({ FRAMES });
}

void TwinFangCardAction::Execute() {
  auto owner = GetOwner();

  // On shoot frame, drop projectile
  auto onFire = [this, owner]() -> void {
    auto tile = GetOwner()->GetTile();

    AUDIO.Play(AudioType::TOSS_ITEM_LITE);

    /**
    
    These duds were written before the grid was surrounded by invisible tiles
    The original games have invisible tiles for items that appear to float or go off the stage
    This can now be re-written to not need twin fang dud objects...
    */
    if (tile->GetY() != 1) {
      TwinFang* twinfang = new TwinFang(GetOwner()->GetField(), GetOwner()->GetTeam(), TwinFang::Type::ABOVE, damage);
      auto props = twinfang->GetHitboxProperties();
      props.aggressor = GetOwnerAs<Character>();
      twinfang->SetHitboxProperties(props);
      twinfang->SetDirection(Direction::right);

      GetOwner()->GetField()->AddEntity(*twinfang, tile->GetX(), tile->GetY() - 1);
    }
    else { // TwinFang floats above the scene
      TwinFang* twinfang = new TwinFang(GetOwner()->GetField(), GetOwner()->GetTeam(), TwinFang::Type::ABOVE_DUD, damage);
      auto props = twinfang->GetHitboxProperties();
      props.aggressor = GetOwnerAs<Character>();
      twinfang->SetHitboxProperties(props);
      twinfang->SetDirection(Direction::right);

      GetOwner()->GetField()->AddEntity(*twinfang, tile->GetX(), tile->GetY());
    }

    if (tile->GetY() != 3) {
      TwinFang* twinfang = new TwinFang(GetOwner()->GetField(), GetOwner()->GetTeam(), TwinFang::Type::BELOW, damage);
      auto props = twinfang->GetHitboxProperties();
      props.aggressor = GetOwnerAs<Character>();
      twinfang->SetHitboxProperties(props);
      twinfang->SetDirection(Direction::right);

      GetOwner()->GetField()->AddEntity(*twinfang, tile->GetX(), tile->GetY() + 1);
    }
    else { // TwinFang floats below the scene
      TwinFang* twinfang = new TwinFang(GetOwner()->GetField(), GetOwner()->GetTeam(), TwinFang::Type::BELOW_DUD, damage);
      auto props = twinfang->GetHitboxProperties();
      props.aggressor = GetOwnerAs<Character>();
      twinfang->SetHitboxProperties(props);
      twinfang->SetDirection(Direction::right);

      GetOwner()->GetField()->AddEntity(*twinfang, tile->GetX(), tile->GetY());
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
  GetOwner()->FreeComponentByID(GetID());
  delete this;
}
