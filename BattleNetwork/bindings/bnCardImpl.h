#pragma once
#ifdef BN_MOD_SUPPORT

class CardAction;
class Character;

namespace Battle
{
    class Card;
    struct CardProperties;
};

class CardImpl {
public:
  virtual ~CardImpl() {};
  virtual CardAction* BuildCardAction(Character* user, Battle::CardProperties& props) = 0;
};

#endif