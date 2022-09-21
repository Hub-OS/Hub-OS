#pragma once

#include "../bnEmotions.h"
#include "../bnInbox.h"

namespace Overworld {
  struct PlayerSession {
    int health{};
    int maxHealth{};
    int money{};
    int megaCardBonus{ 0 }, gigaCardBonus{ 0 }, darkCardBonus{ 0 };
    Emotion emotion{};
    Inbox inbox;
  };
}
