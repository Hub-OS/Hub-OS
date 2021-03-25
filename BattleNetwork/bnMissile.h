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
    Missile(Team _team,Battle::Tile* target, float _duration);
    ~Missile();

    void OnUpdate(double _elapsed) override;
    void OnDelete() override;
    void Attack(Character* _entity) override;
}; 