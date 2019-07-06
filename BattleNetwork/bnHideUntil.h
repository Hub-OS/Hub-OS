#pragma once

#include "bnComponent.h"
#include <SFML/System.hpp>

class Character;

namespace Battle {
    class Tile;
}

/*! \brief Removes any entity from the field until a condition is met
 * 
 * \see HideTimer
 */
class HideUntil : public Component {
public:
  typedef std::function<bool()> Callback; /*!< Query checks for true */

private:
  Battle::Tile* temp;
  Character* owner;
  BattleScene* scene;
  Callback callback;

public:
  /**
   * @brief Sets the query functor
   */
  HideUntil(Character* owner, Callback callback);

  /**
   * @brief When query stored in callback() is true, adds the entity back into the scene
   * @param _elapsed in seconds
   */
  virtual void OnUpdate(float _elapsed);
  
  /**
   * @brief Transfers ownership from entity to the battle scene and removes entity from play
   */
  virtual void Inject(BattleScene&);
};
