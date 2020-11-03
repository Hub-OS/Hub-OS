#include "bnCardAction.h"

class Character;
class SpriteProxyNode;
class RollHeart;

class RollCardAction : public CardAction {
private:
  int damage;
  RollHeart* heart{ nullptr };
public:
  RollCardAction(Character* owner, int damage);
  ~RollCardAction();
  void OnUpdate(float _elapsed);
  void OnAnimationEnd() override;
  void EndAction();
  void Execute();
};