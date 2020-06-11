#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class TwinFangCardAction : public CardAction {
private:
  sf::Sprite twinfang;
  SpriteProxyNode* attachment;
  Animation attachmentAnim;
  int damage;
public:
  TwinFangCardAction(Character* owner, int damage);
  ~TwinFangCardAction();
  void OnUpdate(float _elapsed) override;
  void EndAction() override;
  void Execute() override;
};
