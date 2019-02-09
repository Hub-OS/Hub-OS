#pragma once
#include "bnSpell.h"
#include "bnAnimation.h"

class Thunder : public Spell {
protected:
  Animation animation;
  double elapsed;
  sf::Time timeout;
  Entity* target;

public:
  Thunder(Field* _field, Team _team);
  virtual ~Thunder(void);
  virtual bool CanMoveTo(Battle::Tile* tile);
  virtual void Update(float _elapsed);
  virtual void Attack(Character* _entity);
  virtual vector<Drawable*> GetMiscComponents();
}; 
