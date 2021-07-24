#include "bnHoneyBomberAttackState.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnBees.h"
#include "bnHoneyBomberIdleState.h"
#include "bnAnimationComponent.h"

HoneyBomberAttackState::HoneyBomberAttackState() 
: beeCount(3), attackCooldown(0.4), spawnCooldown(0.4), lastBee(nullptr), AIState<HoneyBomber>() { 
}

HoneyBomberAttackState::~HoneyBomberAttackState() {}

void HoneyBomberAttackState::OnEnter(HoneyBomber& honey) {
  auto animation = honey.GetFirstComponent<AnimationComponent>();

  animation->SetAnimation("ATTACK", Animator::Mode::Loop);
}

void HoneyBomberAttackState::OnUpdate(double _elapsed, HoneyBomber& honey) {
  attackCooldown -= _elapsed;

  if (attackCooldown > 0) return;

  spawnCooldown -= _elapsed;

  if (spawnCooldown <= 0) {
      DoAttack(honey);
      spawnCooldown = 0.4; // reset wait time inbetween spawns
  }
}

void HoneyBomberAttackState::OnLeave(HoneyBomber& honey) {
  honey.EndMyTurn(); // Let the next begin attacking too
  honey.GetFirstComponent<AnimationComponent>()->SetPlaybackSpeed(1.0);
  honey.GetField()->DropNotifier(notifier);
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

    Bees* newBee{ nullptr };
    if (lastBee) {
      newBee = new Bees(*lastBee);
    } else {
      newBee = new Bees(honey.GetTeam(), damage);
    }

    auto props = newBee->GetHitboxProperties();
    props.aggressor = honey.GetID();
    newBee->SetHitboxProperties(props);

    const auto status = honey.GetField()->AddEntity(*newBee, honey.GetTile()->GetX() - 1, honey.GetTile()->GetY());
    if (status != Field::AddEntityStatus::deleted) {
      lastBee = newBee;

      auto onRemove = [this](Entity& target, Entity& observer) {
        if (this->lastBee == &target) {
          this->lastBee = nullptr;
        }
      };

      honey.GetField()->DropNotifier(notifier);
      notifier = honey.GetField()->NotifyOnDelete(lastBee->GetID(), honey.GetID(), onRemove);
    }
  }
}
