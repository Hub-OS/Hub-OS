#pragma once
#include "bnEntity.h"
#include "bnCounterHitPublisher.h"

#include "bnTile.h"

#include <string>
#include <queue>

using std::string;

namespace Hit {
  typedef unsigned char Flags;

  const Flags none = 0x00;
  const Flags recoil = 0x01;
  const Flags shake = 0x02;
  const Flags stun = 0x04;
  const Flags pierce = 0x08;
  const Flags flinch = 0x10;
  const Flags breaking = 0x20;
  const Flags impact = 0x40;
  const Flags drag = 0x80;

  /**
   * @struct Properties
   * @author mav
   * @date 05/05/19
   * @brief Hit box information
   */
  struct Properties {
    int damage;
    Flags flags;
    Element element;
    double secs; // used by both recoil and stun
    Character* aggressor;
    Direction drag; // Used by dragging payload
  };

  const Properties DefaultProperties{ 0, Hit::recoil | Hit::impact, Element::NONE, 3.0, nullptr, Direction::NONE };

}

class DefenseRule;
class Spell;

/**
 * @class Character
 * @author mav
 * @date 05/05/19
 * @brief Characters are mobs, enemies, and players. They have health and can take hits.
 */
class Character : public virtual Entity, public CounterHitPublisher {
  friend class Field;

private:
  bool invokeDeletion; /*!< One-time flag to call OnDelete() if character has custom Delete() behavior */
  bool canShareTile; /*!< Some characters can share tiles with others */
  bool slideFromDrag; /*!< In combat, slides from tiles are cancellable. Slide via drag is not. This flag denotes which one we're in. */

  std::vector<DefenseRule*> defenses; /*<! All defense rules sorted by the lowest priority level */
  
  // Statuses are resolved one property at a time
  // until the entire Flag object is equal to 0x00 None
  // Then we process the next status
  // This continues until all statuses are processed
  std::queue<Hit::Properties> statusQueue;

  sf::Shader* whiteout; /*!< Flash white when hit */
  sf::Shader* stun;     /*!< Flicker yellow with luminance values when stun */
  bool hit; /*!< Was hit this frame */
public:

  /**
   * @class Rank
   * @breif A single definition of a character can have adjustments based on rank
   */
  enum class Rank : const int {
    _1,
    _2,
    _3,
    SP,
    EX,
    Rare1,
    Rare2,
    SIZE
  };

  Character(Rank _rank = Rank::_1);
  virtual ~Character();

  virtual void OnDelete() = 0;
  
  /**
   * @brief Describe what happens to this character when they are hit
   * @param props
   * @return true if hit, false if missed
   */
  virtual const bool OnHit(const Hit::Properties props) = 0;
  
  /**
   * @brief Hit height cannot be easily deduced from sprites and must be implemented
   * @return float
   */
  virtual const float GetHitHeight() const = 0;

  /**
   * The hit routine that happens for every character. Queues status properties and damage
   * to resolve at the end of the battle step.
   * @param props
   * @return Returns false  if IsPassthrough() is true (i-frames), otherwise true
   */
  const bool Hit(Hit::Properties props = Hit::DefaultProperties);

  virtual void ResolveFrameBattleDamage();

  virtual void OnUpdate(float elapsed) = 0;

  // TODO: move tile behavior out of update loop and into its own rule system for customization
  void Update(float elapsed) final;
  
  /**
   * @brief Default characters cannot move onto occupied, broken, or empty tiles
   * @param next
   * @return true if character can move to tile, false otherwise
   */
  virtual bool CanMoveTo(Battle::Tile* next);
  
  /**
   * @brief Sets which frame to make counterable
   * @param frame base 1, frame index to toggle counter flag
   */
  virtual void SetCounterFrame(int frame) {
    ;
  }
  
  /**
   * @brief Get the character's current health
   * @return 
   */
  virtual int GetHealth() const;
  
  /**
   * @brief Get the character's max (init) health
   * @return 
   */
  const int GetMaxHealth() const;

  /**
   * @brief Set the health of the character
   * @param _health
   */
  void SetHealth(int _health);
 
  /**
   * @brief Place the character in the tile's character bucket
   * @param tile
   */
  virtual void AdoptTile(Battle::Tile* tile);

  /**
   * @brief Deletes only if health is <= 0
   */
  void TryDelete();
  
  /**
   * @brief Sets counter flag on
   * @param on
   * 
   * Used by counter frames. 
   * 
   * NOTE: Should be changed to protected or private access
   */
  void ToggleCounter(bool on = true);
  
  /**
   * @brief Stun a character for maxCooldown seconds
   * @param maxCooldown
   * 
   * NOTE: This may not be used anymore. Should probably remove.
   */
  void Stun(double maxCooldown);
  
  /**
   * @brief Query if an attack successfully countered a Character
   * @return true if character is currently countered, false otherwise
   * 
   * NOTE: this may not be used anymore. Should probably remove.
   */
  bool IsCountered();
  
  /**
   * @brief Get the rank of this character
   * @return const Rank
   */
  const Rank GetRank() const;

  /**
   * @brief Some characters allow others to move on top of them
   * @param enabled true, characters can share space, false otherwise
   */
  void ShareTileSpace(bool enabled);
  
  /**
   * @brief Query if entity can share space
   * @return true if shareTilespace is enabled, false otherwise
   */
  const bool CanShareTileSpace() const;

    /**
   * @brief Query if entity is pushable by tiles
   * @return true if canTilePush is enabled, false otherwise
   */
    const bool CanTilePush() const;

  /**
   * @brief Some characters can be moved around on the field by tiles
   * @param enabled
   */
  void ToggleTilePush(bool enabled);

  /**
   * @brief Characters can have names
   * @param name
   */
  void SetName(std::string name);
  
  /**
   * @brief Get name of character
   * @return const std::string
   */
  const std::string GetName() const;

  /**
   * @brief Add defense rule for combat resolution
   * @param rule
   */
  void AddDefenseRule(DefenseRule* rule);
  
  /**
   * @brief Removes the defense rule from this character's defense checks
   * @param rule
   */
  void RemoveDefenseRule(DefenseRule* rule);

  /**
   * @brief Check if spell passes all defense checks
   * @param in attack
   * @return true if passes all defenses
   */
  const bool CheckDefenses(Spell* in);

private:
  int maxHealth;

protected:
  int health;
  bool counterable;
  bool canTilePush;
  std::string name;
  double stunCooldown; /*!< Timer until stun is over */
  Character::Rank rank;
  sf::Time burnCycle; /*!< how long until a tile burns an entity */
  double elapsedBurnTime; // in seconds
};
