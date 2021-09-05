#include "bnCardAction.h"

class Character;
class SpriteProxyNode;
class RollHeart;

class RollCardAction : public CardAction {
private:
  int damage;
  std::shared_ptr<RollHeart> heart{ nullptr };
public:
  RollCardAction(std::shared_ptr<Character> actor, int damage);
  ~RollCardAction();
  void Update(double _elapsed) override;
  void OnAnimationEnd() override;
  void OnActionEnd();
  void OnExecute(std::shared_ptr<Character> user);
};