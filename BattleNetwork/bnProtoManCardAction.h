#include "bnCardAction.h"

class Character;

class ProtoManCardAction : public CardAction {
private:
  int damage;
public:
  ProtoManCardAction(std::shared_ptr<Character> actor, int damage);
  ~ProtoManCardAction();
  void Update(double _elapsed) override;
  void OnAnimationEnd() override;
  void OnActionEnd() override;
  void OnExecute(std::shared_ptr<Character> user) override;
};