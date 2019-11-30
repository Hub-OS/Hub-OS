#include "bnHoneyBomberMoveState.h"
#include "bnMetrid.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnMobMoveEffect.h"
#include "bnAnimationComponent.h"
#include "bnHoneyBomberAttackState.h"

HoneyBomberMoveState::HoneyBomberMoveState() : isMoving(false), moveCount(3), cooldown(1), AIState<HoneyBomber>() { ; }
HoneyBomberMoveState::~HoneyBomberMoveState() { ; }

void HoneyBomberMoveState::OnEnter(HoneyBomber& honey) {
  cooldown = 0.6f;
}

void HoneyBomberMoveState::OnUpdate(float _elapsed, HoneyBomber& honey) {
  if (isMoving) return; // We're already moving (animations take time)

  cooldown -= _elapsed;

  if (cooldown > 0) return; // wait for move cooldown to end

  Battle::Tile* temp = honey.tile;
  Battle::Tile* next = nullptr;

  Entity* target = honey.GetTarget();

  if (target && target->GetTile() && moveCount <= 0 && honey.IsMyTurn()) {
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

  bool moved = (myteam.size() > 0) && honey.Teleport(telex, teley);

  if (moved) {

    auto fx = new MobMoveEffect(honey.GetField());
    honey.GetField()->AddEntity(*fx, honey.GetTile()->GetX(), honey.GetTile()->GetY());

    honey.AdoptNextTile();
    honey.FinishMove();

    //isMoving = true;
    moveCount--;
  }

  // else keep trying to move
  cooldown = 1.0f;
}

void HoneyBomberMoveState::OnLeave(HoneyBomber& met) {

}
