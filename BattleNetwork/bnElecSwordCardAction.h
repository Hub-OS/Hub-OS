#pragma once
#include "bnLongSwordCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class ElecSwordCardAction : public LongSwordCardAction {
public:
  ElecSwordCardAction(Character* owner, int damage);
  ~ElecSwordCardAction();
};

