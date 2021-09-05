#include "bnAlphaElectricState.h"
#include "bnAlphaElectricalCurrent.h"
#include "bnAlphaCore.h"
#include "bnAnimationComponent.h"
#include "bnTile.h"
#include "bnField.h"

AlphaElectricState::AlphaElectricState() 
  : current(nullptr), cooldown(0), count(0), ready(false), AIState<AlphaCore>() { ; }
AlphaElectricState::~AlphaElectricState() { ; }

void AlphaElectricState::OnEnter(AlphaCore& a) {
  ready = false;
  count = 0;
  current = nullptr;

  // skip unless at half health
  if (a.GetHealth() > a.GetMaxHealth() / 2) {
    a.GoToNextState();
    return;
  }

  a.EnableImpervious();
  std::shared_ptr<AnimationComponent> anim = a.GetFirstComponent<AnimationComponent>();

  auto onFinish = [alpha = &a, anim, this]() {
    ready = true;
  };

  anim->SetAnimation("CORE_ATTACK1", onFinish);
}

void AlphaElectricState::OnUpdate(double _elapsed, AlphaCore& a) {
  if (current) {
    if (current->WillRemoveLater()) {
      std::shared_ptr<AnimationComponent> anim = a.GetFirstComponent<AnimationComponent>();
      auto onFinish = [alpha = &a, anim, this]() {
        // This will happen on the core's tick step but we want it to happen immediately after too.
        anim->SetAnimation("CORE_FULL");
        anim->SetPlaybackMode(Animator::Mode::Loop);

        alpha->GoToNextState();
      };

      anim->SetAnimation("CORE_ATTACK1", onFinish);
      anim->SetPlaybackMode(Animator::Mode::Reverse);
      current = nullptr;
      ready = false; // will stop electric currents from being spawned
    }
    return;
  }

  if (!ready) return;

  int count = a.GetRank() == AlphaCore::Rank::EX ? 10 : 7;
  current = std::make_shared<AlphaElectricCurrent>(a.GetTeam(), count);
  auto props = current->GetHitboxProperties();
  props.aggressor = a.GetID();
  current->SetHitboxProperties(props);

  auto state = a.GetField()->AddEntity(current, a.GetTile()->GetX(), a.GetTile()->GetY());
  if (state == Field::AddEntityStatus::deleted) {
    current = nullptr;
  }
}

void AlphaElectricState::OnLeave(AlphaCore& a) {
  a.EnableImpervious(false);
}
