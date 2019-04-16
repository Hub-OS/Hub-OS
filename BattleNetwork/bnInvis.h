#pragma once

#include "bnComponent.h"
#include <SFML/System.hpp>

/* Turns any entity invisible and flags the invisible state property 
   Once the timer runs down, it removes itself from play
*/

class Invis : public Component {
private:
  sf::Time duration;
  float elapsed;

public:
  Invis(Entity* owner);

  virtual void Update(float _elapsed);
  virtual void Inject(BattleScene&);
};