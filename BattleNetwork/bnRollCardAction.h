#include "bnCardAction.h"

class Character;
class SpriteProxyNode;
class RollHeart;

class RollCardAction : public CardAction {
private:
  int damage;
  RollHeart* heart{ nullptr };
public:
  RollCardAction(Character* actor, int damage);
  ~RollCardAction();
  void Update(double _elapsed) override;
  void OnAnimationEnd() override;
  void OnActionEnd();
  void OnExecute(Character* user);
};