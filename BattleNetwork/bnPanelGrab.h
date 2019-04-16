#pragma once
#include "bnSpell.h"
#include "bnTile.h"

class PanelGrab : public Spell {
private:
  sf::Vector2f start;
  double progress;
  double duration;

public:
  PanelGrab(Field* _field, Team _team, float _duration);
  virtual ~PanelGrab(void);

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Character* _entity);
};