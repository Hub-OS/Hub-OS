#include "bnCardAction.h"

class Character;

class AntiDmgCardAction : public CardAction {
private:
  int damage;
public:
  AntiDmgCardAction(Character& owner, int damage);
  ~AntiDmgCardAction();
  void Update(double _elapsed) override;
  void OnAnimationEnd() override;
  void OnEndAction() override;
  void OnExecute() override;
};