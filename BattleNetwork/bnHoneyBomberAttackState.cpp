#include "bnHoneyBomberAttackState.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnBees.h"
#include "bnHoneyBomberIdleState.h"
#include "bnAnimationComponent.h"

HoneyBomberAttackState::HoneyBomberAttackState() 
: beeCount(3), attackCooldown(0.4), spawnCooldown(0.4), lastBee(nullptr), AIState<HoneyBomber>() { 
}

HoneyBomberAttackState::~HoneyBomberAttackState() { ; }

void HoneyBomberAttackState::OnEnter(HoneyBomber& honey) {
  auto animation = honey.GetFirstComponent<AnimationComponent>();

  animation->SetAnimation("ATTACK", Animator::Mode::Loop);
}

void HoneyBomberAttackState::OnUpdate(double _elapsed, HoneyBomber& honey) {
  attackCooldown -= _elapsed;

  if (attackCooldown > 0) return;

  spawnCooldown -= _elapsed;

  bool canAttack = !honey.GetField()->GetAt(honey.GetTile()->GetX() - 1, honey.GetTile()->GetY())->ContainsEntityType<Bees>();
  canAttack = canAttack && spawnCooldown <= 0;

  // we do not want null leaders
  if (lastBee && lastBee->IsDeleted()) {
    lastBee = nullptr;
  }

  if (canAttack) {
      DoAttack(honey);
      spawnCooldown = 0.4; // reset wait time inbetween spawns
  }
}

void HoneyBomberAttackState::OnLeave(HoneyBomber& honey) {
  honey.EndMyTurn(); // Let the next begin attacking too
  honey.GetFirstComponent<AnimationComponent>()->SetPlaybackSpeed(1.0);
}

void HoneyBomberAttackState::DoAttack(HoneyBomber& honey) {
  auto animation = honey.GetFirstComponent<AnimationComponent>();

  if (beeCount-- == 0) {
    animation->CancelCallbacks();

    auto onEnd = [this, m = &honey]() {
      m->ChangeState<HoneyBomberIdleState>();
    };

    animation->SetAnimation("ATTACK", onEnd);
  }
  else {
    int damage = 5; // 5 bees per hit = 25 units of damage total

    Bees* bee;

    if (lastBee) {
      bee = lastBee = new Bees(*lastBee);
    } else {
      bee = new Bees(honey.GetTeam(), damage);
      lastBee = bee;
    }

    auto props = bee->GetHitboxProperties();
    props.aggressor = &honey;
    bee->SetHitboxProperties(props);

    honey.GetField()->AddEntity(*bee, honey.GetTile()->GetX() - 1, honey.GetTile()->GetY());
  }
}
