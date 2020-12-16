#include "bnCardAction.h"

class Character;

class AntiDmgCardAction : public CardAction {
private:
  int damage;
public:
  AntiDmgCardAction(Character& owner, int damage);
  ~AntiDmgCardAction();
  void OnUpdate(double _elapsed);
  void OnAnimationEnd() override;
  void OnEndAction() override;
  void OnExecute() override;
};