#pragma once
#include "bnAIState.h"
#include "bnSpell.h"
class AnimationComponent;

class AlphaCore;
class AlphaElectricCurrent;

class AlphaElectricState : public AIState<AlphaCore>
{
private:
  // in seconds
  float cooldown;
  int count; // Repeats 7 times
  bool ready;
  AlphaElectricCurrent* current;
public:
  AlphaElectricState();
  ~AlphaElectricState();

  void OnEnter(AlphaCore& a);
  void OnUpdate(double _elapsed, AlphaCore& a);
  void OnLeave(AlphaCore& a);
};
