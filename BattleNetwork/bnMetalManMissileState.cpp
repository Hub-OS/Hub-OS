#include "bnMetalManMissileState.h"
#include "bnMetalMan.h"
#include "bnMissile.h"
#include "bnField.h"

MetalManMissileState::MetalManMissileState(int missiles) : cooldown(0.8f), missiles(missiles), AIState<MetalMan>()
{
  lastMissileTimestamp = 0;

  if(missiles > 0) {
    cooldown = missiles * 0.4f;
    lastMissileTimestamp = cooldown;
  }
}


MetalManMissileState::~MetalManMissileState()
{
}

void MetalManMissileState::OnEnter(MetalMan& metal) {
  metal.animationComponent.SetAnimation(MOB_IDLE, Animate::Mode::Loop);
}

void MetalManMissileState::OnUpdate(float _elapsed, MetalMan& metal) {
  cooldown -= _elapsed;

  static int missileIndex = 0;

  if(lastMissileTimestamp - cooldown > 0.4f) {
    lastMissileTimestamp = cooldown;

    if(metal.GetTarget() && metal.GetTarget()->GetTile()) {
        auto tile = metal.GetTarget()->GetTile();

        if(metal.GetHealth() <= 500) {
            if(missileIndex % 2 == 0) {
                tile = metal.GetField()->GetAt(1 + (rand() % 3), 1 + (rand() % 3));
            }
        }

        auto missile = new Missile(metal.GetField(), metal.GetTeam(), tile, 0.4f);
        auto props = missile->GetHitboxProperties();
        props.aggressor = &metal;
        missile->SetHitboxProperties(props);

        missileIndex++;

        metal.GetField()->AddEntity(*missile, metal.GetTile()->GetX(), metal.GetTile()->GetY());
    }
  }

  // printf("cooldown: %f", cooldown);

  if (cooldown < 0) {
      this->ChangeState<MetalManIdleState>();
    }
}

void MetalManMissileState::OnLeave(MetalMan& metal) {}