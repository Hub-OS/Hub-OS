#include "bnCardAction.h"

class Character;

class CubeCardAction : public CardAction {
public:
  CubeCardAction(Character& owner);
  ~CubeCardAction();
  void OnUpdate(double _elapsed);
  void OnAnimationEnd() override;
  void OnEndAction() override;
  void OnExecute() override;
};