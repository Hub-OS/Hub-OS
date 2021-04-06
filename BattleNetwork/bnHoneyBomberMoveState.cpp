#include "bnHoneyBomberMoveState.h"
#include "bnMetrid.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnMobMoveEffect.h"
#include "bnAnimationComponent.h"
#include "bnHoneyBomberAttackState.h"

HoneyBomberMoveState::HoneyBomberMoveState() : AIState<HoneyBomber>() { ; }
HoneyBomberMoveState::~HoneyBomberMoveState() { ; }

void HoneyBomberMoveState::OnEnter(HoneyBomber& honey) {
  cooldown = 0.6;
}

void HoneyBomberMoveState::OnUpdate(double _elapsed, HoneyBomber& honey) {
  if (isMoving) return; // animations take time...

  cooldown -= _elapsed;

  if (cooldown > 0) return; // wait for move cooldown to end

  Battle::Tile* temp = honey.tile;
  Battle::Tile* next = nullptr;

  Entity* target = honey.GetTarget();

  bool isAggro = honey.GetFirstComponent<AnimationComponent>()->GetPlaybackSpeed() != 1.0;
  if (target && target->GetTile() && isAggro)
 {
    // Try attacking
    return honey.ChangeState<HoneyBomberAttackState>();
  }

  auto myteam = honey.GetField()->FindTiles([&honey](Battle::Tile* t) {
    return t->GetTeam() == honey.GetTeam();
  });

  int telex = honey.GetTile()->GetX();
  int teley = honey.GetTile()->GetY();

  if (myteam.size() > 0) {
      int randIndex = rand() % myteam.size();
      telex = myteam[randIndex]->GetX();
      teley = myteam[randIndex]->GetY();
  }

  auto onMove = [honeyPtr = &honey, this] {
    this->isMoving = true;
    auto fx = new MobMoveEffect();
    honeyPtr->GetField()->AddEntity(*fx, *honeyPtr->GetTile());
    return honeyPtr->ChangeState<HoneyBomberIdleState>();
  };

  Battle::Tile* destTile = honey.GetField()->GetAt(telex, teley);
  bool queued = honey.Teleport(destTile, ActionOrder::voluntary, onMove);

  // else repeat 
  if (!queued) {
    return honey.ChangeState<HoneyBomberIdleState>();
  }
}

void HoneyBomberMoveState::OnLeave(HoneyBomber& met) {

}

void HoneyBomberMoveState::OnMoveEvent(const MoveEvent& event)
{
}
