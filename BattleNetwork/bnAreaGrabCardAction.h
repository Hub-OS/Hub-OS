#include "bnCardAction.h"

class Character;
class PanelGrab;

class AreaGrabCardAction : public CardAction {
private:
  int damage;
  PanelGrab* panelPtr{ nullptr };
public:
  AreaGrabCardAction(Character& owner, int damage);
  ~AreaGrabCardAction();
  void Update(double _elapsed) override;
  void OnAnimationEnd() override;
  void OnEndAction() override;
  void OnExecute() override;
};