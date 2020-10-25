#include "bnCardAction.h"

class Character;

class CubeCardAction : public CardAction {
public:
  CubeCardAction(Character* owner);
  ~CubeCardAction();
  void OnUpdate(float _elapsed);
  void OnAnimationEnd() override;
  void EndAction();
  void Execute();
};