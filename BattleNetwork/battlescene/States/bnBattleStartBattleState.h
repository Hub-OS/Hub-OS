#pragma once

#include "bnBattleTextIntroState.h"
#include "../../bnText.h"
#include "../../frame_time_t.h"

#include <SFML/Graphics/Sprite.hpp>
#include <Swoosh/Timer.h>
/*
    \brief This state handles the battle start message that appears

    depending on the type of battle (Round-based PVP? PVE?)
*/

class Player;

struct BattleStartBattleState final : public BattleTextIntroState {
  BattleStartBattleState();
  void onStart(const BattleSceneState* last) override;
};
