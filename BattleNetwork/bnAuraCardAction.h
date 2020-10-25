#include "bnCardAction.h"
#include "bnAura.h"

class Character;

class AuraCardAction : public CardAction {
private:
  Aura::Type type;
public:
  AuraCardAction(Character* owner, Aura::Type type);
  ~AuraCardAction();
  void OnUpdate(float _elapsed);
  void OnAnimationEnd() override;
  void EndAction();
  void Execute();
};