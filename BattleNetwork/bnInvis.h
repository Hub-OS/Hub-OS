#pragma once

#include "bnComponent.h"
#include <SFML/System.hpp>

<<<<<<< HEAD
/* Turns any entity invisible and flags the invisible state property 
   Once the timer runs down, it removes itself from play
=======
/*! \brief Turns any entity invisible and flags the passthrough state property 
 *
 * Once the timer runs down, it removes itself from play
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
*/

class Invis : public Component {
private:
<<<<<<< HEAD
  sf::Time duration;
  float elapsed;

public:
  Invis(Entity* owner);

  virtual void Update(float _elapsed);
=======
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
  virtual void Update(float _elapsed);
  
  /**
   * @brief does not inject
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  virtual void Inject(BattleScene&);
};