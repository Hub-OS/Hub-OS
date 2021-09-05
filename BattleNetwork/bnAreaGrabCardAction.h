#include "bnCardAction.h"

class Character;
class PanelGrab;

class AreaGrabCardAction : public CardAction {
private:
  int damage;
  std::shared_ptr<PanelGrab> panelPtr{ nullptr };
public:
  AreaGrabCardAction(std::shared_ptr<Character> actor, int damage);
  ~AreaGrabCardAction();
  void Update(double _elapsed) override;
  void OnAnimationEnd() override;
  void OnActionEnd() override;
  void OnExecute(std::shared_ptr<Character> user) override;
};