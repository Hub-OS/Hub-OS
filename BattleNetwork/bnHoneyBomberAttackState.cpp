#include "bnHoneyBomberAttackState.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnBees.h"
#include "bnHoneyBomberIdleState.h"
#include "bnAnimationComponent.h"

HoneyBomberAttackState::HoneyBomberAttackState() : AIState<HoneyBomber>() { 
  beeCount = 3; 
  attackCooldown = 0.1f;
}
HoneyBomberAttackState::~HoneyBomberAttackState() { ; }

void HoneyBomberAttackState::OnEnter(HoneyBomber& honey) {
  auto animation = honey.GetFirstComponent<AnimationComponent>();

  animation->SetAnimation("ATTACK", Animator::Mode::Loop);
}

void HoneyBomberAttackState::OnUpdate(float _elapsed, HoneyBomber& honey) {
  attackCooldown -= _elapsed;

  if (attackCooldown > 0.f) return;

  bool canAttack = !honey.GetField()->GetAt(honey.GetTile()->GetX() - 1, honey.GetTile()->GetY())->ContainsEntityType<Bees>();

  if (canAttack) {
      DoAttack(honey);
  }
}

void HoneyBomberAttackState::OnLeave(HoneyBomber& honey) {
  honey.EndMyTurn(); // Let the next begin attacking too
}

void HoneyBomberAttackState::DoAttack(HoneyBomber& honey) {
  auto animation = honey.GetFirstComponent<AnimationComponent>();

  if (--beeCount == 0) {
    animation->CancelCallbacks();

    auto onEnd = [this, m = &honey]() {
      m->ChangeState<HoneyBomberIdleState>();
    };

    animation->SetAnimation("ATTACK", onEnd);
  }
  else {
    int damage = 25;

    auto bees = new Bees(honey.GetField(), honey.GetTeam(), damage);
    auto props = bees->GetHitboxProperties();
    props.aggressor = &honey;
    bees->SetHitboxProperties(props);

    honey.GetField()->AddEntity(*bees, honey.GetTile()->GetX() - 1, honey.GetTile()->GetY());
  }
}
