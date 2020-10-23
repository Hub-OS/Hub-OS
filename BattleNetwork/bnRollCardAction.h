#include "bnCardAction.h"

class Character;
class SpriteProxyNode;

class RollCardAction : public CardAction {
private:
  int damage;
public:
  RollCardAction(Character* owner, int damage);
  ~RollCardAction();
  void OnUpdate(float _elapsed);
  void OnAnimationEnd() override;
  void EndAction();
  void Execute();
};