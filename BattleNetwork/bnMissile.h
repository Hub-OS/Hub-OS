#pragma once
#include "bnSpell.h"
#include "bnAnimationComponent.h"

namespace Battle {
  class Tile;
};

class Missile : public Spell {
private:
    sf::Vector2f start;
    Battle::Tile* target;

    double progress;
    double duration;

    bool goingUp;

    AnimationComponent* anim;
public:
    Missile(Field* _field, Team _team, Battle::Tile* target, float _duration);
    virtual ~Missile();

    virtual void OnUpdate(float _elapsed);
    virtual bool Move(Direction _direction);
    virtual void Attack(Character* _entity);
}; 