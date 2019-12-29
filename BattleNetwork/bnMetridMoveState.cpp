#include "bnMetridMoveState.h"
#include "bnMetrid.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnMobMoveEffect.h"
#include "bnAnimationComponent.h"
#include "bnMetridAttackState.h"

MetridMoveState::MetridMoveState() : isMoving(false), moveCount(5), cooldown(1), AIState<Metrid>() { ; }
MetridMoveState::~MetridMoveState() { ; }

void MetridMoveState::OnEnter(Metrid& met) {
  if (met.GetRank() == Metrid::Rank::_2) {
    cooldown = 0.6f;
  }
  else if (met.GetRank() == Metrid::Rank::_3) {
    cooldown = 0.4f;
  }
}

void MetridMoveState::OnUpdate(float _elapsed, Metrid& met) {
  if (isMoving) return; // We're already moving (animations take time)

  cooldown -= _elapsed;

  if (cooldown > 0) return; // wait for move cooldown to end

  Battle::Tile* temp = met.tile;
  Battle::Tile* next = nullptr;

  Entity* target = met.GetTarget();

  if (target && target->GetTile() && moveCount <= 0 && met.IsMyTurn()) {
    // Try attacking
    return met.ChangeState<MetridAttackState>();
  }

  auto myteam = met.GetField()->FindTiles([&met](Battle::Tile* t) {
    if (t->GetTeam() == met.GetTeam() && t->IsWalkable())
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

  bool moved = (myteam.size() > 0) && met.Teleport(telex, teley);

  if (moved) {

    auto fx = new MobMoveEffect(met.GetField());
    met.GetField()->AddEntity(*fx, met.GetTile()->GetX(), met.GetTile()->GetY());

    met.AdoptNextTile();
    met.FinishMove();

    //isMoving = true;
    moveCount--;
  }

  // else keep trying to move
  if (met.GetRank() == Metrid::Rank::_1) {
    cooldown = 1.0f;
  } else   if (met.GetRank() == Metrid::Rank::_2) {
    cooldown = 0.6f;
  }
  else if (met.GetRank() == Metrid::Rank::_3) {
    cooldown = 0.4f;
  }
}

void MetridMoveState::OnLeave(Metrid& met) {

}

