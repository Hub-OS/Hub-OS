#include "bnCardAction.h"

class Character;

class AntiDmgCardAction : public CardAction {
private:
  int damage;
public:
  AntiDmgCardAction(std::shared_ptr<Character> actor, int damage);
  ~AntiDmgCardAction();
  void OnAnimationEnd() override;
  void OnActionEnd() override;
  void OnExecute(std::shared_ptr<Character> user) override;
};