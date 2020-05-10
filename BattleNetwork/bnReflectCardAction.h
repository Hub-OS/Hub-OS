#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class ReflectCardAction : public CardAction {
  int damage;

public:
  ReflectCardAction(Character& owner, int damage);
  ~ReflectCardAction();
  void OnEndAction() override;
  void OnExecute() override;
};
