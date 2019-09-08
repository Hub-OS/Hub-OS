#pragma once
#include "bnSpell.h"
#include "bnAnimationComponent.h"

class Vulcan : public Spell {
public:

  Vulcan(Field* _field, Team _team, int _damage);
  virtual ~Vulcan();

  virtual void OnUpdate(float _elapsed);

  virtual bool CanMoveTo(Battle::Tile* next);

  /**
   * @brief Deal impact damage
   * @param _entity
   */
  virtual void Attack(Character* _entity);
private:
  bool spawnGuard;
  bool hit;
  Character* contact;
  int damage;
  float cooldown;
  float random; // offset
  float hitHeight;
  sf::Texture* texture;
  float progress;
  AnimationComponent* animationComponent;
};