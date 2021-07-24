#pragma once

#include "bnCardAction.h"
#include "bnAnimation.h"
#include "bnField.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class BusterCardAction : public CardAction {
private:
  SpriteProxyNode* buster{ nullptr }, * flare{ nullptr };
  Attachment* busterAttachment{ nullptr };
  Field* field{ nullptr };
  Animation busterAnim, flareAnim;
  bool charged{};
  int damage{};
  Field::NotifyID_t notifier{ -1 };
public:
  BusterCardAction(Character& user, bool charged, int damage);
  ~BusterCardAction();

  void Update(double _elapsed) override;
  void OnAnimationEnd();
  void OnActionEnd();
  void OnExecute(Character* user);
};
