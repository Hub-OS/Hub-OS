#include "bnMegalianIdleState.h"
#include "bnTile.h"
#include "bnField.h"

MegalianIdleState::MegalianIdleState() : AIState<Megalian>() { ; }
MegalianIdleState::~MegalianIdleState() { ; }

void MegalianIdleState::OnEnter(Megalian& m) {
}

void MegalianIdleState::OnUpdate(double _elapsed, Megalian& m) {
  /* Nothing, just wait the animation out*/
}

void MegalianIdleState::OnLeave(Megalian& m) {
}

