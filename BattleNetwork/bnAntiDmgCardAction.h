#include "bnCardAction.h"

class Character;

class AntiDmgCardAction : public CardAction {
private:
  int damage;
public:
  AntiDmgCardAction(Character& actor, int damage);
  ~AntiDmgCardAction();
  void OnAnimationEnd() override;
  void OnActionEnd() override;
  void OnExecute(Character* user) override;
};