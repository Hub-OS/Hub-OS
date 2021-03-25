#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include "bnFireBurn.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class FireBurnCardAction : public CardAction {
private:
  SpriteProxyNode* attachment;
  Animation attachmentAnim;
  FireBurn::Type type;
  int damage;
  bool crackTiles{ true };
public:
  FireBurnCardAction(Character& owner, FireBurn::Type type, int damage);
  ~FireBurnCardAction();

  void Update(double _elapsed) override;
  void OnAnimationEnd() override;
  void OnEndAction() override;
  void OnExecute() override;
  void CrackTiles(bool state);
};
