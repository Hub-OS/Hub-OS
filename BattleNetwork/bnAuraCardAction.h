#include "bnCardAction.h"
#include "bnAura.h"

class Character;

class AuraCardAction : public CardAction {
private:
  Aura::Type type;
public:
  AuraCardAction(Character* actor, Aura::Type type);
  ~AuraCardAction();
  void Update(double _elapsed);
  void OnAnimationEnd() override;
  void OnActionEnd() override;
  void OnExecute(Character* user) override;
};