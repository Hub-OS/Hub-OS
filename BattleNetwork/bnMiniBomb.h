#pragma once
#include "bnSpell.h"

class MiniBomb : public Spell {
private:
  int random;
  sf::Vector2f start;
  double arcDuration;
  double arcProgress;

public:
  MiniBomb(Field* _field, Team _team, sf::Vector2f startPos, float _duration, int damage);
  ~MiniBomb();

  void OnUpdate(double _elapsed) override;
  bool Move(Direction _direction) override;
  void Attack(Character* _entity) override;
  void OnDelete() override;
};