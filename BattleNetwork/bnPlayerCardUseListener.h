/*! \brief Describes what happens when a card is used by the player navi 
 * 
 * \warning This is based on old system design and will be removed completely
 * 
 *       Reasons for removal:
 *       No reason why this needs to be separate from the enemy card listener
 *       The code is nearly identical and should be refactored to support being
 *       used by both players and enemies.
 * 
 *       Also cards will be scripted and this logic will be handled by the 
 *       script interpreter (Lua) and this will no longer be used
 */

#pragma once

#include "bnAudioResourceManager.h"
#include "bnCardUseListener.h"
#include "bnPlayer.h"
#include "bnCard.h"

class PlayerCardUseListener : public CardUseListener {
private:
  Player * player; /*!< Entity to listen for */
  
public:
  PlayerCardUseListener(Player& _player) : CardUseListener() { player = &_player;  }

  /**
   * @brief What happens when a card is used
   * @param card Card used
   * @param character Character using card
   */
  void OnCardUse(const Battle::Card& card, Character& character, long long timestamp);
};
