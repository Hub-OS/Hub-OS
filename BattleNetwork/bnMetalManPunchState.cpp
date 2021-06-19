#include "bnMetalManPunchState.h"
#include "bnMetalMan.h"
#include "bnHitbox.h"
#include "bnObstacle.h"
#include "bnAudioResourceManager.h"

MetalManPunchState::MetalManPunchState() : AIState<MetalMan>()
{
}


MetalManPunchState::~MetalManPunchState()
{
}

void MetalManPunchState::OnEnter(MetalMan& metal) {
  auto tile = metal.GetTarget()? metal.GetTarget()->GetTile() : nullptr;
  if (!tile) {
    metal.GoToNextState();
    return;
  }

  auto anim = metal.GetFirstComponent<AnimationComponent>();

  reserved = metal.GetField()->GetAt(tile->GetX() + 1, tile->GetY());

  if (reserved) {
    metal.canEnterRedTeam = true;

    auto lastTile = metal.GetTile();
    reserved->ReserveEntityByID(metal.GetID());
    lastTile->ReserveEntityByID(metal.GetID());

    auto onFinish = [metal = &metal, lastTile, anim, this]() {
      Logger::Log("metalman move on finish called");

      if (metal->Teleport(reserved)) {

        auto onFinishPunch = [m = metal, anim, lastTile]() {
          auto onFinishMove = [m, anim, lastTile] {
            
            m->Teleport(lastTile, ActionOrder::immediate);
            m->GoToNextState();
          };
          anim->SetAnimation("MOVING", onFinishMove);
        };
        auto onGroundHit = [this, m = metal]() {
          Attack(*m);
        };

        anim->SetAnimation("PUNCH", onFinishPunch);
        anim->SetCounterFrameRange(1, 3);
        anim->AddCallback(4, onGroundHit, true);
      }
      else {
        metal->GoToNextState();
      }
    };

    anim->SetAnimation("MOVING", onFinish);
    metal.canEnterRedTeam = true;
  }
  else {
    metal.GoToNextState();
  }

}

void MetalManPunchState::OnLeave(MetalMan& metal) {
  reserved && metal.GetTile() != reserved? reserved->RemoveEntityByID(metal.GetID()) : 0;
  auto anim = metal.GetFirstComponent<AnimationComponent>();
  anim->SetAnimation("IDLE");
}

void MetalManPunchState::OnUpdate(double _elapsed, MetalMan& metal) {

}

void MetalManPunchState::Attack(MetalMan& metal) {
  Battle::Tile* tile = metal.GetField()->GetAt(metal.GetTile()->GetX()-1, metal.GetTile()->GetY());

  if (tile) {
    Entity* next = nullptr;

    Hitbox* hitbox = new Hitbox(metal.GetTeam(), 100);
    auto props = hitbox->GetHitboxProperties();
    props.flags |= Hit::flash;
    props.aggressor = metal.GetID();
    hitbox->SetHitboxProperties(props);

    metal.field->AddEntity(*hitbox, tile->GetX(), tile->GetY());

    if (tile->GetState() != TileState::empty && tile->GetState() != TileState::broken) {
      metal.EventChannel().Emit(&Camera::ShakeCamera, 5.0, sf::seconds(0.5));
      metal.Audio().Play(AudioType::PANEL_CRACK);

      if (tile->GetState() == TileState::cracked) {
        tile->SetState(TileState::broken);
      }
      else {
        tile->SetState(TileState::cracked);
      }
    }
  }
}
