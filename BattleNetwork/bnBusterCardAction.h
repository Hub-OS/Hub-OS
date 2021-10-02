#pragma once

#include "bnCardAction.h"
#include "bnAnimation.h"
#include "bnField.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class BusterCardAction : public CardAction {
private:
  SpriteProxyNode buster, flare;
  Attachment* busterAttachment{ nullptr };
  std::shared_ptr<Field> field{ nullptr };
  Animation busterAnim, flareAnim;
  bool charged{};
  int damage{};
  Field::NotifyID_t notifier{ -1 };
public:
  BusterCardAction(std::shared_ptr<Character> user, bool charged, int damage);

  void Update(double _elapsed) override;
  void OnAnimationEnd();
  void OnActionEnd();
  void OnExecute(std::shared_ptr<Character> user);
};
