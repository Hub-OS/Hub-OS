#include "bnAlphaIdleState.h"
#include "bnTile.h"
#include "bnField.h"

AlphaIdleState::AlphaIdleState() : AIState<AlphaCore>() { ; }
AlphaIdleState::~AlphaIdleState() { ; }

void AlphaIdleState::OnEnter(AlphaCore& a) {
}

void AlphaIdleState::OnUpdate(float _elapsed, AlphaCore& a) {

}

void AlphaIdleState::OnLeave(AlphaCore& a) {
}
