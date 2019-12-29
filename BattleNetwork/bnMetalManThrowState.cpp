#include "bnMetalManThrowState.h"
#include "bnMetalMan.h"
#include "bnMetalBlade.h"
#include "bnAudioResourceManager.h"

MetalManThrowState::MetalManThrowState() : AIState<MetalMan>()
{
}


MetalManThrowState::~MetalManThrowState()
{
}

void MetalManThrowState::OnEnter(MetalMan& metal) {
  auto onFinish = [m = &metal]() { m->GoToNextState(); };
  auto onThrow = [this, m = &metal]() { this->Attack(*m); };

  metal.SetAnimation("THROW", onFinish);
  metal.SetCounterFrame(1);
  metal.SetCounterFrame(2);
  metal.OnFrameCallback(3, onThrow, Animator::NoCallback, true);
}

void MetalManThrowState::OnLeave(MetalMan& metal) {
}

void MetalManThrowState::OnUpdate(float _elapsed, MetalMan& metal) {

}

void MetalManThrowState::Attack(MetalMan& metal) {
  Spell* blade = new MetalBlade(metal.GetField(), metal.GetTeam(), 1.0);
  auto props = blade->GetHitboxProperties();
  props.aggressor = &metal;
  blade->SetHitboxProperties(props);

  blade->SetDirection(Direction::LEFT);

  metal.GetField()->AddEntity(*blade, metal.GetTile()->GetX()-1, metal.GetTile()->GetY());
  AUDIO.Play(AudioType::SWORD_SWING);
}