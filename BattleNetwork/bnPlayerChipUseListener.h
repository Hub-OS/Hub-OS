/*! \brief Describes what happens when a chip is used by the player navi 
 * 
 * \warning This is based on old system design and will be removed completely
 * 
 *       Reasons for removal:
 *       No reason why this needs to be separate from the enemy chip listener
 *       The code is nearly identical and should be refactored to support being
 *       used by both players and enemies.
 * 
 *       Also chips will be scripted and this logic will be handled by the 
 *       script interpreter (Lua) and this will no longer be used
 */

#pragma once

#include "bnAudioResourceManager.h"
#include "bnChipUseListener.h"
#include "bnPlayer.h"
#include "bnChip.h"

class PlayerChipUseListener : public ChipUseListener {
private:
  Player * player; /*!< Entity to listen for */

public:
  PlayerChipUseListener(Player* _player) : ChipUseListener() { player = _player; }
  PlayerChipUseListener(Player& _player) : ChipUseListener() { player = &_player;  }

  /**
   * @brief What happens when a chip is used
   * @param chip Chip used
   * @param character Character using chip
   */
  void OnChipUse(Chip& chip, Character& character);
};
