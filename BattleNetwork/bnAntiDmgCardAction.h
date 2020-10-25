#include "bnCardAction.h"

class Character;

class AntiDmgCardAction : public CardAction {
private:
  int damage;
public:
  AntiDmgCardAction(Character* owner, int damage);
  ~AntiDmgCardAction();
  void OnUpdate(float _elapsed);
  void OnAnimationEnd() override;
  void EndAction();
  void Execute();
};