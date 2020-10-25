#include "bnCardAction.h"

class Character;

class AreaGrabCardAction : public CardAction {
private:
  int damage;
public:
  AreaGrabCardAction(Character* owner, int damage);
  ~AreaGrabCardAction();
  void OnUpdate(float _elapsed);
  void OnAnimationEnd() override;
  void EndAction();
  void Execute();
};