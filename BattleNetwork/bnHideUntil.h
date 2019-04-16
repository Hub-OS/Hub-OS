#pragma once

#include "bnComponent.h"
#include <SFML/System.hpp>

/* Removes any entity from the field until a condition is met
*/

class HideUntil : public Component {
public:
  typedef std::function<bool()> Callback;

private:
  sf::Time duration;
  float elapsed;
  Battle::Tile* temp;
  Character* owner;
  BattleScene* scene;
  Callback callback;

public:
  HideUntil(Character* owner, Callback callback);

  virtual void Update(float _elapsed);
  virtual void Inject(BattleScene&);
};
