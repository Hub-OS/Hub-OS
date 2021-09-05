#pragma once

#include "bnCharacter.h"
#include "bnFalzarIdleState.h"
#include "bnFalzarMoveState.h"
#include "bnFalzarRoarState.h"
#include "bnFalzarSpinState.h"
#include "bnBossPatternAI.h"

class Falzar : public Character, public BossPatternAI<Falzar> {
public:
  Falzar(Rank rank = Rank::_1);

  friend class FalzarIdleState;
  friend class FalzarMoveState;
  friend class FalzarRoarState;
  friend class FalzarSpinState;
  using DefaultState = FalzarIdleState;

  /**
   * @brief Uses AI state to move around. Deletes when health is below zero.
   * @param _elapsed in seconds
   */
  void OnUpdate(double _elapsed) override;

  void OnDelete() override;

  bool CanMoveTo(Battle::Tile* next) override;

  /**
   * @brief Get the hit height of this entity
   * @return const float
   */
   // const float GetHeight() const override;

private:
  double animProgress{};
  Animation animation;
  std::shared_ptr<DefenseRule> bossBody;
  SpriteProxyNode
    head, body, guardLeft, guardRight,
    wingLeft, wingRight, tail, legLeft, legRight,
    shadow;

  int testState{};
  void Idle();
  void Roar();
  void MoveAnim();
  void Flap();
  void UpdateNodeAnims(double elapsed);
};