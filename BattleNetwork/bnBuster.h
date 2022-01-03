#pragma once
#include "bnSpell.h"
#include "bnAnimationComponent.h"

/**
 * @class Buster
 * @author mav
 * @date 05/05/19
 * @brief Classic buster attack 
 * 
 * NOTE: This comes from legacy code and could be improved
 */
class Buster : public Spell {
public:
  /**
   * @brief If _charged is true, deals 10 damage
   */
  Buster(Team _team,bool _charged, int damage);
  ~Buster() override;

  void Init() override;

  void OnUpdate(double _elapsed) override;
  
  bool CanMoveTo(Battle::Tile* next) override;

  void OnDelete() override;

  void OnCollision(const std::shared_ptr<Entity>) override;

  /**
   * @brief Deal impact damage
   * @param _entity
   */
  void Attack(std::shared_ptr<Entity> _entity) override;

private:
  bool isCharged;
  bool spawnGuard;
  std::shared_ptr<Character> contact;
  int damage;
  frame_time_t cooldown;
  float random; // offset
  float hitHeight;
  std::shared_ptr<sf::Texture> texture;
  float progress;
  std::shared_ptr<AnimationComponent> animationComponent;
};