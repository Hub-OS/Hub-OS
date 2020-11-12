#pragma once

#include "bnComponent.h"
#include "bnDefenseInvis.h"
#include <SFML/System.hpp>

/*! \brief Turns any entity invisible and flags the passthrough state property 
 *
 * Once the timer runs down, it removes itself from play
*/

class Invis : public Component {
private:
  sf::Time duration; /*!< Set to 15 seconds */
  float elapsed; /*!< Time passed in seconds */
  DefenseInvis* defense;
public:
  /**
   * @brief attach to an entity 
   */
  Invis(Entity* owner);

  ~Invis();

  /**
   * @brief When under time, set opacity to 50% and pasthrough. Otherwise, restore character.
   * @param _elapsed in seconds
   */
  void OnUpdate(float _elapsed) override;
  
  /**
   * @brief does not inject
   */
  void Inject(BattleSceneBase&) override;
};