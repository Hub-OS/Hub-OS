#include "bnCardAction.h"

class Character;

class ProtoManCardAction : public CardAction {
private:
  int damage;
public:
  ProtoManCardAction(Character& owner, int damage);
  ~ProtoManCardAction();
  void OnUpdate(float _elapsed);
  void OnAnimationEnd() override;
  void OnEndAction() override;
  void OnExecute() override;
};