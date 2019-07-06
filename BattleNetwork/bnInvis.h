#pragma once

#include "bnComponent.h"
#include <SFML/System.hpp>

/*! \brief Turns any entity invisible and flags the passthrough state property 
 *
 * Once the timer runs down, it removes itself from play
*/

class Invis : public Component {
private:
  sf::Time duration; /*!< Set to 15 seconds */
  float elapsed; /*!< Time passed in seconds */

public:
  /**
   * @brief attach to an entity 
   */
  Invis(Entity* owner);

  /**
   * @brief When under time, set opacity to 50% and pasthrough. Otherwise, restore character.
   * @param _elapsed in seconds
   */
  virtual void OnUpdate(float _elapsed);
  
  /**
   * @brief does not inject
   */
  virtual void Inject(BattleScene&);
};