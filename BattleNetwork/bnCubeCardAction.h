#include "bnCardAction.h"

class Character;

class CubeCardAction : public CardAction {
public:
  CubeCardAction(Character& owner);
  ~CubeCardAction();
  void Update(double _elapsed) override;
  void OnAnimationEnd() override;
  void OnEndAction() override;
  void OnExecute() override;
};