#include "bnCardAction.h"

class Character;

class CubeCardAction : public CardAction {
public:
  CubeCardAction(std::shared_ptr<Character> actor);
  ~CubeCardAction();
  void Update(double _elapsed) override;
  void OnAnimationEnd() override;
  void OnActionEnd() override;
  void OnExecute(std::shared_ptr<Character> user) override;
};