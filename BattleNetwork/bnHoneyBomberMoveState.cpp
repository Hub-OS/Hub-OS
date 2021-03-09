#include "bnHoneyBomberMoveState.h"
#include "bnMetrid.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnMobMoveEffect.h"
#include "bnAnimationComponent.h"
#include "bnHoneyBomberAttackState.h"

HoneyBomberMoveState::HoneyBomberMoveState() : moveCount(3), cooldown(1), AIState<HoneyBomber>() { ; }
HoneyBomberMoveState::~HoneyBomberMoveState() { ; }

void HoneyBomberMoveState::OnEnter(HoneyBomber& honey) {
  cooldown = 0.6;
}

void HoneyBomberMoveState::OnUpdate(double _elapsed, HoneyBomber& honey) {
  cooldown -= _elapsed;

  if (cooldown > 0) return; // wait for move cooldown to end

  Battle::Tile* temp = honey.tile;
  Battle::Tile* next = nullptr;

  Entity* target = honey.GetTarget();

  if (target && target->GetTile() && honey.GetFirstComponent<AnimationComponent>()->GetPlaybackSpeed() != 1.0)
 {
    // Try attacking
    return honey.ChangeState<HoneyBomberAttackState>();
  }

  auto myteam = honey.GetField()->FindTiles([&honey](Battle::Tile* t) {
    if (t->GetTeam() == honey.GetTeam())
      return true;

    return false;
  });

  int telex = 0;
  int teley = 0;

  if (myteam.size() > 0) {
      int randIndex = rand() % myteam.size();
      telex = myteam[randIndex]->GetX();
      teley = myteam[randIndex]->GetY();
  }

  Battle::Tile* destTile = honey.GetField()->GetAt(telex, teley);
  bool queued = (myteam.size() > 0) && honey.Teleport(destTile);

  if (honey.IsMoving(&lastcheck)) {

    auto fx = new MobMoveEffect();
    honey.GetField()->AddEntity(*fx, *honey.GetTile());
    moveCount--;
  }

  // else repeat 
  return honey.ChangeState<HoneyBomberIdleState>();
}

void HoneyBomberMoveState::OnLeave(HoneyBomber& met) {

}

void HoneyBomberMoveState::OnMoveEvent(const MoveEvent& event)
{
}
