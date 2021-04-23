#include "bnCardAction.h"

class Character;

class ProtoManCardAction : public CardAction {
private:
  int damage;
public:
  ProtoManCardAction(Character& actor, int damage);
  ~ProtoManCardAction();
  void Update(double _elapsed) override;
  void OnAnimationEnd() override;
  void OnActionEnd() override;
  void OnExecute(Character* user) override;
};