#include "bnMetridAttackState.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnMeteor.h"
#include "bnMetridIdleState.h"
#include "bnAnimationComponent.h"

MetridAttackState::MetridAttackState() : AIState<Metrid>() { meteorCooldown = 0; meteorCount = 5; beginAttack = false; }
MetridAttackState::~MetridAttackState() { ; }

void MetridAttackState::OnEnter(Metrid& met) {
  auto animation = met.GetFirstComponent<AnimationComponent>();

  auto onFinish = [this, animation, &met]() {
    if (met.GetRank() == Metrid::Rank::_1) {
      animation->SetAnimation("MOB_HIDDEN_1");
    }
    else if (met.GetRank() == Metrid::Rank::_2) {
      animation->SetAnimation("MOB_HIDDEN_2");
      meteorCount = 8;
    }
    else if (met.GetRank() == Metrid::Rank::_3) {
      animation->SetAnimation("MOB_HIDDEN_3");
      meteorCount = 11;
    }

    animation->SetPlaybackMode(Animator::Mode::Loop);
    this->DoAttack(met);
  };

  if (met.GetRank() == Metrid::Rank::_1) {
    animation->SetAnimation("MOB_HIDE_1", onFinish);
  }
  else if (met.GetRank() == Metrid::Rank::_2) {
    animation->SetAnimation("MOB_HIDE_2", onFinish);
    meteorCount = 8;
  }
  else if (met.GetRank() == Metrid::Rank::_3) {
    animation->SetAnimation("MOB_HIDE_3", onFinish);
    meteorCount = 11;
  }
}

void MetridAttackState::OnUpdate(float _elapsed, Metrid& met) {
  if (met.GetTarget()) {
    auto last = met.GetTarget()->GetTile();

    if (last != nullptr) {
      target = last;
    }
  }

  if (beginAttack) {
    meteorCooldown -= _elapsed;

    if (meteorCooldown < 0) {
      DoAttack(met);
    }
  }
}

void MetridAttackState::OnLeave(Metrid& met) {
  met.NextMetridTurn(); // Let the next begin attacking too
}

void MetridAttackState::DoAttack(Metrid& met) {
  beginAttack = true; // we are now ready to launch meteors in an interval

  auto animation = met.GetFirstComponent<AnimationComponent>();

  if (--meteorCount == 0) {
    animation->CancelCallbacks();

    auto onEnd = [this, m = &met]() {
      m->ChangeState<MetridIdleState>();
    };

    if (met.GetRank() == Metrid::Rank::_1) {
      animation->SetAnimation("MOB_HIDE_1", onEnd);
    }
    else if (met.GetRank() == Metrid::Rank::_2) {
      animation->SetAnimation("MOB_HIDE_2", onEnd);
    }
    else if (met.GetRank() == Metrid::Rank::_3) {
      animation->SetAnimation("MOB_HIDE_3", onEnd);
    }

    animation->SetPlaybackMode(Animator::Mode::Reverse);
  }
  else {
    int damage = 40;
    meteorCooldown = 0.8f;

    if (met.GetRank() == Metrid::Rank::_2) {
      damage = 80;
      meteorCooldown = 0.6f;
    }
    else if (met.GetRank() == Metrid::Rank::_3) {
      damage = 120;
      meteorCooldown = 0.4f;
    }

    auto meteor = new Meteor(met.GetField(), met.GetTeam(), target, damage, 0.4f);
    auto props = meteor->GetHitboxProperties();
    props.aggressor = &met;
    meteor->SetHitboxProperties(props);

    met.GetField()->AddEntity(*meteor, target->GetX(),target->GetY());
  }
}
