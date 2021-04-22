#include "bnMetalManThrowState.h"
#include "bnMetalMan.h"
#include "bnMetalBlade.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"

MetalManThrowState::MetalManThrowState() : AIState<MetalMan>()
{
}


MetalManThrowState::~MetalManThrowState()
{
}

void MetalManThrowState::OnEnter(MetalMan& metal) {
  auto onFinish = [m = &metal]() { m->GoToNextState(); };
  auto onThrow = [this, m = &metal]() { Attack(*m); };

  auto anim = metal.GetFirstComponent<AnimationComponent>();
  anim->SetAnimation("THROW", onFinish);
  anim->SetCounterFrameRange(1, 2);
  anim->AddCallback(3, onThrow, true);
}

void MetalManThrowState::OnLeave(MetalMan& metal) {
}

void MetalManThrowState::OnUpdate(double _elapsed, MetalMan& metal) {

}

void MetalManThrowState::Attack(MetalMan& metal) {
  Spell* blade = new MetalBlade(metal.GetTeam(), 1.0);
  auto props = blade->GetHitboxProperties();
  props.aggressor = metal.GetID();
  blade->SetHitboxProperties(props);

  blade->SetDirection(Direction::left);

  metal.GetField()->AddEntity(*blade, metal.GetTile()->GetX()-1, metal.GetTile()->GetY());
  metal.Audio().Play(AudioType::SWORD_SWING);
}