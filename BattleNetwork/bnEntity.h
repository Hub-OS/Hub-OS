/*! \brief Base class for all content that exists on the battle field
 * 
 *  \class Entity
 * 
 * Moving on a battle field consisting of a 6x3 grid requires specific
 * behavior. To appear authentic, the entities that exist on the field
 * must abide by special movement rules. Movement is on a per-tile basis
 * and entities can only appear on one tile at a time. 
 * 
 * Entities by themselves respond to no collision and knows nothing
 * about health points. Entities only know about the tile they are on,
 * their element if any, their team, their passthrough properties,
 * and maintain a list of Components that may alter their behavior in 
 * battle.
 */

#pragma once
#include <string>
#include <vector>
using std::string;

#include "bnAnimation.h"
#include "bnDirection.h"
#include "bnTeam.h"
#include "bnEngine.h"
#include "bnTextureType.h"
#include "bnElements.h"
#include "bnComponent.h"
#include "bnCallback.h"

namespace Battle {
  class Tile;
}

class Field;
class BattleScene; // forward decl

class Entity : public SpriteProxyNode {
  friend class Field;
  friend class Component;
  friend class BattleScene;

private:
  long ID;              /*!< IDs are used for tagging during battle & to identify entities in scripting. */
  static long numOfIDs; /*!< Internal counter to identify the next entity with. */
  int alpha;            /*!< Control the transparency of an entity. */
  long lastComponentID; /*!< Entities keep track of new components to run through scene injection later. */
  bool hasSpawned;      /*!< Flag toggles true when the entity is first placed onto the field. Calls OnSpawn(). */
  float height;         /*!< Height of the entity relative to tile floor. Used for visual effects like projectiles or for hitbox detection*/
public:
  using DeleteCallback = Callback<void()>;

  Entity();
  virtual ~Entity();

  virtual void OnDelete() = 0;

  void Spawn(Battle::Tile& start);

  virtual void OnSpawn(Battle::Tile& start) { };
  virtual void OnBattleStart() { };
  virtual void OnBattleStop() { };

  /**
   * @brief Virtual. Update an entity. Used by Character class. @see Character::Update()
   * This must be used by Character base class to perform correctly. All entities slide.
   * Sliding can be used for entities that always glide over tiles like Boomerangs or Fishy
   * but they are also used for entities that sometime slide like when traveling over
   * Ice or being pushed like Cubes. This update routine correctly checks the movement
   * state and determines when to accurately move the entity from its current Tile pointer
   * to the next Tile pointer. 
   * 
   * After sliding, the next Tile pointer will be set to null.
   * Also, if the entitiy does not have FloatShoe or AirShoe, sliding will be set to false
   * only if the next tile isn't also Ice. Otherwise, the entity will continue to slide.
   * 
   * To recreate entities that slide at all times, poll IsSliding() in your Update method
   * and if it is false, use SlideToTile(true) to slide again. @see Gear::Update()
   * 
   * @param _elapsed Amount of time elapsed since last frame in seconds.
   */
  virtual void Update(float _elapsed);
  
  /**
   * @brief Virtual. Move to another tile given a direction.
   * @param _direction The direction to try and move to
   * @return true if the next tile pointer was able to move, false if obstructed.
   */
  virtual bool Move(Direction _direction);
  
  /**
   * @brief Virtual. Move to any tile on the field given it's column and row number.
   * @param x The tile's column
   * @param y The tile's row
   * @return true if the next tile pointer was able to move, false if obstructed.
   */
  virtual bool Teleport(int x, int y);
  
  /**
   * @brief Virtual. Queries if an entity can move to a target tile.
   * This function dictates the rules of when an entity can move.
   * Some implementation exampes are when bosses can teleport to the player's area
   * only when their state permits it. It will return true in that case but false
   * when the boss is not attacking and shall only be allowed to roam on their 
   * team's side.
   * @param next The tile to query.
   * @return true if the entity can move to the tile, false if not permitted.
   */
  virtual bool CanMoveTo(Battle::Tile* next);

  /**
   * @brief Entity's ID
   * @return ID 
   */
  const long GetID() const;

  /**
   * @brief Checks to see if the input team is friendly 
   * @param _team
   * @return true if the input team is the same or friendly, false if they are opposing teams
   */
  bool Teammate(Team _team);

  /**
   * @brief Changes the tile pointer to the input tile
   * Directly changing the tile pointer is not recommended. @see Teleport() @see Move()
   * The input tile claims ownership of the entity and the entity's old tile releases ownership
   * @param _tile
   */
  void SetTile(Battle::Tile* _tile);
  
  /**
   * @brief Get the current tile pointer
   * The tile pointer refers to the tile the entity is currently standing in
   * @return Tile pointer
   */
  Battle::Tile* GetTile() const;
  
  /**
   * @brief Get the next tile pointer
   * The next tile pointer refer to the tile the entity is attempting to tranfer to
   * @return Tile pointer
   */
  const Battle::Tile * GetNextTile() const;
  
  /**
   * @brief Changes the behavior in the movement resolve step to slide to the next tile
   */
  void SlideToTile(bool );
  
  /**
   * @brief Checks if entity is sliding
   * @return true if sliding, false if no longer sliding
   */
  const bool IsSliding() const;

  /**
   * @brief Sets the field pointer
   * @param _field
   */
  void SetField(Field* _field);

  /**
   * @brief Gets the field pointer
   * Useful to check field state such as IsTimeFrozen()
   * @return Field pointer
   */
  Field* GetField() const;

  /**
   * @brief Gets the entity's assigned team
   * @return Current Team
   */
  Team GetTeam() const;
  
  /**
   * @brief Assigns team 
   * @param _team the team to assign
   */
  void SetTeam(Team _team);

  /**
   * @brief Attacks skip entities with passthrough enabled.
   * @param state set true for passthrough, set false to be tangible
   */
  void SetPassthrough(bool state);
  
  /**
   * @brief Get the passthrough state
   * @return true if passthrough, false if tangible
   */
  bool IsPassthrough();

  /**
   * @brief Sets the opacity level of this entity
   * @param value range from 0-255 
   */
  void SetAlpha(int value);

  /**
   * @brief Enable or disable FloatShoe
   * @param state set true to enable, false to disable
   */
  void SetFloatShoe(bool state);
  
  /**
   * @brief Enable or disable AirShoe
   * @param state set true to enable, false to disable
   */
  void SetAirShoe(bool state);
  
  /**
   * @brief Query if entity has FloatShoe enabled
   * @return true if Floatshoe is enabled, false otherwise
   */
  bool HasFloatShoe();
  
  /**
   * @brief Query if entity has AirShoe enabled
   * @return true if AirShoe is enabled, false otherwise
   */
  bool HasAirShoe();

  /**
   * @brief Directly modify the entity's current direction property. @see Entity::Move() 
   * Direction can be used for entities that use a single direction for linear movement.
   * e.g. Mettaur Waves, Bullet, Fishy
   * @param direction the new direction
   */
  void SetDirection(Direction direction);
  
  /**
   * @brief Query the entity's current direction
   * @return The entity's current direction
   */
  Direction GetDirection();
  
  /**
   * @brief Query the entity's previous direction
   * Can be used to see if an entity was previously moving before it changed direction
   * @see Gear::Update()
   * @return The entity's previous direction
   */
  Direction GetPreviousDirection();

  /**
   * @brief Frees and clears all components attached. Sets `delete` flag to true.
   */
  void Delete();

  /**
  * @brief Builds and returns a reference to a callback function of type void()
  * Useful for safely determining the lifetime of another entity in play
  */
  DeleteCallback& CreateDeleteCallback();
  
  /**
   * @brief Query if an entity has been deleted but not removed this frame
   * @return true if flagged for deletion, false otherwise
   */
  bool IsDeleted() const;

  /**
   * @brief Changes the element of the entity
   * Elements change combat outcomes and some tile effects
   * @param _elem the element to change to
   */
  void SetElement(Element _elem);
  
  /**
   * @brief Query the element of the entity
   * @return Returns the current element
   */
  const Element GetElement() const;
  
  /**
   * @brief Takes an input elements and returns if the element is super effective against this entity
   * Wood > Electric > Aqua > Fire > Wood 
   * @param _other the other element
   * @return true if super effective, false if not
   */
  const bool IsSuperEffective(Element _other) const;
  
  /**
   * @brief Forces the entity to adopt the next tile pointer 
   */
  void AdoptNextTile();
  
  /**
   * @brief Pure virtual. Must be defined by super classes of Entity.
   * 
   * This is a visitor design pattern with double distpatching
   * The super class must call tile->AddEntity(*this) to force the overloaded type
   * in the Tile class so that the entity is added to the correct type bucket @see Tile
   * @param tile to adopt
   */
  virtual void AdoptTile(Battle::Tile* tile) = 0;

  /**
   * @brief Sets the entity's TFC flag
   * Useful in update loop so that enemies will not attack when
   * the screen is paused or the cards are open
   * @param state true to freeze time, false if ongoing
   */
  void ToggleTimeFreeze(bool state);
  
  /**
   * @brief Query if entity is time frozen
   * @return true if entity is time frozen, false if active
   */
  const bool IsTimeFrozen();

  /**
   * @brief Get the first component that matches the exact Type
   * @return null if no component is found, otherwise returns the component
   */
  template<typename Type>
  Type* GetFirstComponent();

   /**
   * @brief Get all components that matches the exact Type
   * @return vector of specified components
   */
  template<typename Type>
  std::vector<Type*> GetComponents();

  /**
* @brief Get all components that inherit BaseType
* @return vector of related components
*/
  template<typename BaseType>
  std::vector<BaseType*> GetComponentsDerivedFrom();

  /**
  * @brief Check if entity is a specialized type
  * @return true if entity could be dynamically casted to Type
  * @warning dynamic casts are costly! Avoid if possible!
  */
  template<typename Type>
  bool IsA();

  /**
   * @brief Creates and attaches a component to an entity
   * @param Args. Parameter pack of any argument type to pass into the component's constructor
   * @return Returns the component as a pointer of the same type as ComponentT
   */
  template<typename ComponentT, typename... Args>
  ComponentT* CreateComponent(Args&&...);

  /**
  * @brief Attaches a component to an entity
  * @param c the component to add
  * @return Returns the component as a pointer of the common base class type
  */
  Component* RegisterComponent(Component* c);
  
  /**
   * @brief Frees all attached components from an owner
   * All attached components owner pointer will be null
   */
  void FreeAllComponents();
  
  /**
   * @brief Frees one component with the same ID
   * @param ID ID of the component to remove
   */
  void FreeComponentByID(long ID);

  /**
   * @brief Hit height ca be overwritten to deduce from sprite bounds
   * @return float
   */
  virtual const float GetHeight() const;

  virtual void SetHeight(const float height);

  /**
  * @brief used by move systems to signal a move is complete 
  * use with animation component to complete a move animation after teleporting to the next tile
  * otherwise the move system will incorrectly deduce the move states
  */
  void FinishMove();

protected:
  Battle::Tile* next; /**< Pointer to the next tile */
  Battle::Tile* tile; /**< Current tile pointer */
  Battle::Tile* previous; /**< Entities retain a previous pointer in case they need to be moved back */
  sf::Vector2f tileOffset; /**< All entities draw at the center of the tile + tileOffset*/
  sf::Vector2f slideStartPosition; /**< Used internally when sliding*/
  Field* field;
  Team team;
  Element element;

  std::vector<Component*> components; /*!< List of all components attached to this entity*/
  std::vector<DeleteCallback> deleteCallbacks;

  void SetSlideTime(sf::Time time);

  const int GetMoveCount() const; /*!< Total intended movements made. Used to calculate rank*/

private:
  bool isTimeFrozen;
  bool ownedByField; /*!< Must delete the entity manual if not owned by the field. */
  bool passthrough;
  bool floatShoe;
  bool airShoe;
  bool isSliding; /*!< If sliding/gliding to a tile */
  bool deleted;
  int moveCount; /*!< Used by battle results */
  sf::Time slideTime; /*!< how long slide behavior lasts */
  sf::Time defaultSlideTime; /*!< If slidetime is modified by outside source, the slide to return back to */
  double elapsedSlideTime; /*!< When elapsedSlideTime is equal to slideTime, slide is over */
  Direction direction;
  Direction previousDirection;

    /**
   * @brief Used internally before moving and updates the start position vector used in the sliding motion
   */
  void UpdateSlideStartPosition();
};

template<typename Type>
inline Type* Entity::GetFirstComponent()
{
  for (vector<Component*>::iterator it = components.begin(); it != components.end(); ++it) {
    if (typeid(*(*it)) == typeid(Type)) {
      return dynamic_cast<Type*>(*it);
    }
  }

  return nullptr;
}

template<typename Type>
inline std::vector<Type*> Entity::GetComponents()
{
  auto res = std::vector<Type*>();

  for (vector<Component*>::iterator it = components.begin(); it != components.end(); ++it) {
    if (typeid(*(*it)) == typeid(Type)) {
      res.push_back(dynamic_cast<Type*>(*it));
    }
  }

  return res;
}


template<typename BaseType>
inline std::vector<BaseType*> Entity::GetComponentsDerivedFrom()
{
  auto res = std::vector<BaseType*>();

  for (vector<Component*>::iterator it = components.begin(); it != components.end(); ++it) {
    BaseType* cast = dynamic_cast<BaseType*>(*it);

    if (cast) {
      res.push_back(cast);
    }
  }

  return res;
}

template<typename Type>
inline bool Entity::IsA() {
  return (dynamic_cast<Type*>(this) != nullptr);
}

template<typename ComponentT, typename... Args>
inline ComponentT* Entity::CreateComponent(Args&& ...args) {
    ComponentT* c = new ComponentT(std::forward<decltype(args)>(args)...);

    components.push_back(c);

    // Newest components appear first in the list for easy referencing
    std::sort(components.begin(), components.end(), [](Component* a, Component* b) { return a->GetID() > b->GetID(); });

    return c;
}