#pragma once
#include "bnSpell.h"

/*! \brief Hitbox is an invisible spell used for more complex attack behaviors or used as body hitboxes
 * 
 * Because some attacks have lagging behavior, dropping a hitbox is more effective design
 * than allowing on the attack itself to deal damage. Some bosses can be moved into as well
 * and need to account for hitting the player. Overall hitbox is a spell that can be used as a tool
 * to design more complex attacks that do not behave in a simple way 
 */
class Hitbox : public Spell {
private:
  int damage; /*!< how many units of damage to deal */
  bool hit; /*!< Flag if hit last frame */
  std::function<void(std::shared_ptr<Character>)> attackCallback{}; /*!< Optional callback when Attack() is triggered*/
  std::function<void(const std::shared_ptr<Character>)> collisionCallback{};
public:
  /**
   * @brief disables tile highlighting by default
   */
  Hitbox(Team _team, int damage = 0);
  
  /**
   * @brief deconstructor
   */
  ~Hitbox();

  void OnSpawn(Battle::Tile& start) override;

  /**
   * @brief Attacks tile and deletes itself
   * @param _elapsed in seconds
   */
  void OnUpdate(double _elapsed) override;
  
  /**
   * @brief Deals hitbox property damage and then deletes itself
   * @param _entity entity to attack
   */
  void Attack(std::shared_ptr<Character> _entity) override;

  void OnCollision(const std::shared_ptr<Character> entity) override;

  /**
  * @brief Some hitboxes perform actions on hit and this is the best place to add them
  */
  void AddCallback(std::function<void(std::shared_ptr<Character>)> attackCallback, std::function<void(const std::shared_ptr<Character>)> collisionCallback = [](const std::shared_ptr<Character>) {});

  void OnDelete() override;
};
