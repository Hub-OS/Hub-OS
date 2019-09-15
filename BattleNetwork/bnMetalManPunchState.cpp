#include "bnMetalManPunchState.h"
#include "bnMetalMan.h"
#include "bnHitBox.h"
#include "bnObstacle.h"
#include "bnAudioResourceManager.h"

MetalManPunchState::MetalManPunchState() : AIState<MetalMan>()
{
}


MetalManPunchState::~MetalManPunchState()
{
}

void MetalManPunchState::OnEnter(MetalMan& metal) {
  auto onFinish = [m = &metal]() { m->ChangeState<MetalManIdleState>(); };
  auto onGroundHit = [this, m = &metal]() {   this->Attack(*m); };

  metal.SetAnimation("PUNCH", onFinish);
  metal.SetCounterFrame(1);
  metal.SetCounterFrame(2);
  metal.SetCounterFrame(3);
  metal.OnFrameCallback(4, onGroundHit, std::function<void()>(), true);
}

void MetalManPunchState::OnLeave(MetalMan& metal) {
}

void MetalManPunchState::OnUpdate(float _elapsed, MetalMan& metal) {

}

void MetalManPunchState::Attack(MetalMan& metal) {
  Battle::Tile* tile = metal.GetField()->GetAt(metal.GetTile()->GetX()-1, metal.GetTile()->GetY());

  if (tile) {
    Entity* next = nullptr;

    HitBox* hitbox = new HitBox(metal.field, metal.GetTeam(), 100);
    auto props = hitbox->GetHitboxProperties();
    props.flags |= Hit::flinch;
    props.aggressor = &metal;
    hitbox->SetHitboxProperties(props);

    metal.field->AddEntity(*hitbox, tile->GetX(), tile->GetY());

    if (tile->GetState() != TileState::EMPTY && tile->GetState() != TileState::BROKEN) {
      ENGINE.GetCamera()->ShakeCamera(5.0, sf::seconds(0.5));
      AUDIO.Play(AudioType::PANEL_CRACK);

      if (tile->GetState() == TileState::CRACKED) {
        tile->SetState(TileState::BROKEN);
      }
      else {
        tile->SetState(TileState::CRACKED);
      }
    }
  }
}