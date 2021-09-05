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
  FireBurnCardAction(std::shared_ptr<Character> actor, FireBurn::Type type, int damage);
  ~FireBurnCardAction();

  void Update(double _elapsed) override;
  void OnAnimationEnd() override;
  void OnActionEnd() override;
  void OnExecute(std::shared_ptr<Character> user) override;
  void CrackTiles(bool state);
};
