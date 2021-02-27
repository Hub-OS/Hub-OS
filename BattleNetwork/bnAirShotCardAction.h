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
  AirShotCardAction(Character& user, int damage);
  ~AirShotCardAction();

  void Update(double _elapsed) override;
  void OnAnimationEnd() override;
  void OnEndAction() override;
  void OnExecute() override;
};
