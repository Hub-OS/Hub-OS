#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class AirShotCardAction : public CardAction {
private:
  SpriteProxyNode* attachment;
  Animation attachmentAnim;
  int damage;
public:
  AirShotCardAction(std::shared_ptr<Character> actor, int damage);
  ~AirShotCardAction();

  void Update(double _elapsed) override;
  void OnAnimationEnd() override;
  void OnActionEnd() override;
  void OnExecute(std::shared_ptr<Character> user) override;
};
