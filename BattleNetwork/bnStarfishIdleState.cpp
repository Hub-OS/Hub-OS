#include "bnStarfishIdleState.h"
#include "bnWave.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnStarfishAttackState.h"

StarfishIdleState::StarfishIdleState() : cooldown(3), AIState<Starfish>() { ; }
StarfishIdleState::~StarfishIdleState() { ; }

void StarfishIdleState::OnEnter(Starfish& star) {
  star.animationComponent.SetAnimation("IDLE", Animate::Mode::Loop);
}

void StarfishIdleState::OnUpdate(float _elapsed, Starfish& star) {
  if (cooldown < 0) {
<<<<<<< HEAD
=======
    // Attack 3 times
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
    this->ChangeState<StarfishAttackState>(3);
  }

  cooldown -= _elapsed;
}

void StarfishIdleState::OnLeave(Starfish& star) {
}
