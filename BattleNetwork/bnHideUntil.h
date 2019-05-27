#pragma once

#include "bnComponent.h"
#include <SFML/System.hpp>

<<<<<<< HEAD
/* Removes any entity from the field until a condition is met
*/

class HideUntil : public Component {
public:
  typedef std::function<bool()> Callback;

private:
  sf::Time duration;
  float elapsed;
=======
/*! \brief Removes any entity from the field until a condition is met
 * 
 * \see HideTimer
 */
class HideUntil : public Component {
public:
  typedef std::function<bool()> Callback; /*!< Query checks for true */

private:
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  Battle::Tile* temp;
  Character* owner;
  BattleScene* scene;
  Callback callback;

public:
<<<<<<< HEAD
  HideUntil(Character* owner, Callback callback);

  virtual void Update(float _elapsed);
=======
  /**
   * @brief Sets the query functor
   */
  HideUntil(Character* owner, Callback callback);

  /**
   * @brief When query stored in callback() is true, adds the entity back into the scene
   * @param _elapsed in seconds
   */
  virtual void Update(float _elapsed);
  
  /**
   * @brief Transfers ownership from entity to the battle scene and removes entity from play
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  virtual void Inject(BattleScene&);
};
