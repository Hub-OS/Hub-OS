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

  metal.canEnterRedTeam = true;

  auto anim = metal.GetFirstComponent<AnimationComponent>();

  auto nextTile = metal.GetField()->GetAt(tile->GetX() + 1, tile->GetY());

  if (nextTile) {
    auto lastTile = metal.GetTile();
    nextTile->ReserveEntityByID(metal.GetID());
    lastTile->ReserveEntityByID(metal.GetID());


    auto onFinish = [metal = &metal, nextTile, lastTile, anim, this]() {
      Logger::Log("metalman move on finish called");

      if (metal->Teleport(nextTile)) {

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
  }
  else {
    metal.GoToNextState();
  }

}

void MetalManPunchState::OnLeave(MetalMan& metal) {
  auto anim = metal.GetFirstComponent<AnimationComponent>();
  anim->SetAnimation("IDLE");
  metal.canEnterRedTeam = false;
}

void MetalManPunchState::OnUpdate(double _elapsed, MetalMan& metal) {

}

void MetalManPunchState::Attack(MetalMan& metal) {
  Battle::Tile* tile = metal.GetField()->GetAt(metal.GetTile()->GetX()-1, metal.GetTile()->GetY());

  if (tile) {
    Entity* next = nullptr;

    Hitbox* hitbox = new Hitbox(metal.GetTeam(), 100);
    auto props = hitbox->GetHitboxProperties();
    props.flags |= Hit::flinch;
    props.aggressor = metal.GetID();
    hitbox->SetHitboxProperties(props);

    metal.field->AddEntity(*hitbox, tile->GetX(), tile->GetY());

    if (tile->GetState() != TileState::empty && tile->GetState() != TileState::broken) {
      metal.EventBus().Emit(&Camera::ShakeCamera, 5.0, sf::seconds(0.5));
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
