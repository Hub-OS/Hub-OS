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
    std::shared_ptr<AnimationComponent> anim;

public:
    Missile(Team _team,Battle::Tile* target, float _duration);
    ~Missile();

    void OnUpdate(double _elapsed) override;
    void OnDelete() override;
    void Attack(std::shared_ptr<Character> _entity) override;
}; 