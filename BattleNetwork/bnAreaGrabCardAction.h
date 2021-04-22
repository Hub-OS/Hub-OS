#include "bnCardAction.h"

class Character;
class PanelGrab;

class AreaGrabCardAction : public CardAction {
private:
  int damage;
  PanelGrab* panelPtr{ nullptr };
public:
  AreaGrabCardAction(Character& actor, int damage);
  ~AreaGrabCardAction();
  void Update(double _elapsed) override;
  void OnAnimationEnd() override;
  void OnEndAction() override;
  void OnExecute(Character* user) override;
};