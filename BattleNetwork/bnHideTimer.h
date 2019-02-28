#pragma once

#include "bnComponent.h"
#include <SFML/System.hpp>

/* REmoves any entity from the field for a duration of time
   Once the timer runs down, it removes itself from play
*/

class HideTimer : public Component {
private:
  sf::Time duration;
  float elapsed;
  Battle::Tile* temp;
  Character* owner;
  BattleScene* scene;
public:
  HideTimer(Character* owner, double secs);

  virtual void Update(float _elapsed);
  virtual void Inject(BattleScene&);
}; 
