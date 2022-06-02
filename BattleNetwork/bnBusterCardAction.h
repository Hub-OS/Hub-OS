#pragma once

#include "bnCardAction.h"
#include "bnAnimation.h"
#include "bnField.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class BusterCardAction : public CardAction {
private:
  Attachment* busterAttachment{ nullptr };
  bool charged{}, canMove{};
  int damage{};
  std::shared_ptr<SpriteProxyNode> buster;

  frame_time_t CalculateCooldown(unsigned speedLevel, unsigned tileDist);

public:
  BusterCardAction(std::weak_ptr<Character> user, bool charged, int damage);
  void OnExecute(std::shared_ptr<Character> user) override;
  void OnActionEnd() override;
  void OnAnimationEnd() override;
  bool CanMoveInterrupt();
};
