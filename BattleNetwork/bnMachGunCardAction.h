#include "bnCardAction.h"
#include "bnAnimation.h"
#include "bnSpriteProxyNode.h"

class MachGunCardAction : public CardAction {
  int damage{};
  SpriteProxyNode machgun;
  Animation machgunAnim;

public:
  MachGunCardAction(Character& owner, int damage);
  ~MachGunCardAction();

  void Execute() override;
  void EndAction() override;
  void OnAnimationEnd() override;
};