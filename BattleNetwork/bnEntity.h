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
#include <functional>
using std::string;

#include "bnResourceHandle.h"
#include "bnAnimation.h"
#include "bnDirection.h"
#include "bnTeam.h"
#include "bnGame.h"
#include "bnTextureType.h"
#include "bnElements.h"
#include "bnComponent.h"
#include "bnCallback.h"
#include "bnEventBus.h"
#include "bnActionQueue.h"

namespace Battle {
  class Tile;
}

class Field;
class BattleSceneBase; // forward decl

struct EntityComparitor {
  bool operator()(Entity* f, Entity* s) const;
};

class Entity : public SpriteProxyNode, public ResourceHandle {
public:
  using ID_t = long;

  friend class Field;
  friend class Component;
  friend class BattleSceneBase;

private:
  ID_t ID{};              /*!< IDs are used for tagging during battle & to identify entities in scripting. */
  static long numOfIDs; /*!< Internal counter to identify the next entity with. */
  int alpha{ 255 };            /*!< Control the transparency of an entity. */
  Component::ID_t lastComponentID{}; /*!< Entities keep track of new components to run through scene injection later. */
  bool hasSpawned{ false };      /*!< Flag toggles true when the entity is first placed onto the field. Calls OnSpawn(). */
  float height{0};         /*!< Height of the entity relative to tile floor. Used for visual effects like projectiles or for hitbox detection */
  bool isUpdating{ false }; /*!< If an entity has updated once this frame, skip some update routines */
  EventBus::Channel channel; /*!< Our event bus channel to emit events */

  /**
   * @brief Frees one component with the same ID
   * @param ID ID of the component to remove
   */
  void SortComponents();
  void ClearPendingComponents();
  void ReleaseComponentsPendingRemoval();
  void InsertComponentsPendingRegistration();
public:

  using RemoveCallback = Callback<void()>;

  Entity();
  virtual ~Entity();

  /**
  * @brief Performs some user-specified deletion behavior before removing from play
  *
  * Deleted entities are excluded from all battle attack steps however they 
  * will be visually present and must be removed from field by calling Remove()
  */
  virtual void OnDelete() = 0;

  /**
  * @brief Sets the tile for this entity and invokes OnSpawn() if this is the first time
  */
  void Spawn(Battle::Tile& start);

  virtual void OnSpawn(Battle::Tile& start) { };

  /**
  * @brief Performs some user-specified behavior when the battle starts for the first time 
  * @example See bnGears.h for an entity that is live when the round begins and only moves after this call.
  */
  virtual void OnBattleStart() { };

  /**
  * @brief Performs some user-specified behavior when the battle is over
  * @example See bnGears.h for an entity that only stops in place when the round is over
  */
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
  virtual void Update(double _elapsed);
  
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
  const ID_t GetID() const;

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
  * @brief Flags the entity to be pruned from the field 
  */
  void Remove();

  /**
  * @brief Builds and returns a reference to a callback function of type void()
  * Useful for safely determining the lifetime of another entity in play
  */
  std::reference_wrapper<RemoveCallback> CreateRemoveCallback();
  
  /**
   * @brief Query if an entity has been deleted but not removed this frame
   * @return true if flagged for deletion, false otherwise
   */
  bool IsDeleted() const;

  /**
  * @brief Query if an entity has been marked for removal
  * @return true if flagged, false otherwise
  */
  bool WillRemoveLater() const;

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
  template<typename ComponentType>
  ComponentType* GetFirstComponent() const;

   /**
   * @brief Get all components that matches the exact Type
   * @return vector of specified components
   */
  template<typename ComponentType>
  std::vector<ComponentType*> GetComponents() const;

  /**
* @brief Get all components that inherit BaseType
* @return vector of related components
*/
  template<typename BaseType>
  std::vector<BaseType*> GetComponentsDerivedFrom() const;

  /**
  * @brief Check if entity is a specialized type
  * @return true if entity could be dynamically casted to Type
  * @warning dynamic casts are costly! Avoid if possible!
  */
  template<typename Type>
  bool IsA();

  /**
   * @brief Creates and then registers a component to an entity
   * @param Args. Parameter pack of any argument type to pass into the component's constructor
   * @return Returns the component as a pointer of the same type as ComponentT
   */
  template<typename ComponentType, typename... Args>
  ComponentType* CreateComponent(Args&&...);

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
  * @brief Returns a channel in the EventBus
  * @return Returns the channel as const reference
  */
  const EventBus::Channel& EventBus() const;
  
  /**
   * @brief Frees one component with the same ID
   * @param ID ID of the component to remove
   */
  void FreeComponentByID(Component::ID_t ID);

  /**
   * @brief Hit height can be overwritten to deduce from sprite bounds
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

  virtual void QueueAction(const ActionEvent& action) = 0;
  virtual void EndCurrentAction() = 0;
  void ClearActionQueue();

protected:
  Battle::Tile* next{ nullptr }; /**< Pointer to the next tile */
  Battle::Tile* tile{ nullptr }; /**< Current tile pointer */
  Battle::Tile* previous{ nullptr }; /**< Entities retain a previous pointer in case they need to be moved back */
  sf::Vector2f tileOffset{ 0,0 }; /**< All entities draw at the center of the tile + tileOffset*/
  sf::Vector2f slideStartPosition{ 0,0 }; /**< Used internally when sliding*/
  Field* field{ nullptr };
  Team team{};
  Element element{Element::none};
  ActionQueue actionQueue;
  std::vector<Component*> components; /*!< List of all components attached to this entity*/

  struct ComponentBucket {
    Component* pending{ nullptr };
    enum class Status : unsigned {
      add,
      remove
    };

    Status action{ Status::add };
  };
  std::list<ComponentBucket> queuedComponents;
  std::vector<RemoveCallback> removeCallbacks;

  void SetSlideTime(sf::Time time);

  const int GetMoveCount() const; /*!< Total intended movements made. Used to calculate rank*/

private:
  bool isTimeFrozen{};
  bool ownedByField{}; /*!< Must delete the entity manual if not owned by the field. */
  bool passthrough{};
  bool floatShoe{};
  bool airShoe{};
  bool isSliding{}; /*!< If sliding/gliding to a tile */
  bool deleted{}; /*!< Used to trigger OnDelete() callback and exclude entity from most update routines*/
  bool flagForRemove{}; /*!< Used to remove this entity from the field immediately */
  int moveCount{}; /*!< Used by battle results */
  sf::Time slideTime; /*!< how long slide behavior lasts */
  sf::Time defaultSlideTime; /*!< If slidetime is modified by outside source, the slide to return back to */
  double elapsedSlideTime{}; /*!< When elapsedSlideTime is equal to slideTime, slide is over */
  Direction direction{};
  Direction previousDirection{};

    /**
   * @brief Used internally before moving and updates the start position vector used in the sliding motion
   */
  void UpdateSlideStartPosition();
};

template<typename ComponentType>
inline ComponentType* Entity::GetFirstComponent() const
{
  for (vector<Component*>::const_iterator it = components.begin(); it != components.end(); ++it) {
    if (typeid(*(*it)) == typeid(ComponentType)) {
      return dynamic_cast<ComponentType*>(*it);
    }
  }

  return nullptr;
}

template<typename ComponentType>
inline std::vector<ComponentType*> Entity::GetComponents() const
{
  auto res = std::vector<ComponentType*>();

  for (vector<Component*>::const_iterator it = components.begin(); it != components.end(); ++it) {
    if (typeid(*(*it)) == typeid(ComponentType)) {
      res.push_back(dynamic_cast<ComponentType*>(*it));
    }
  }

  return res;
}


template<typename BaseType>
inline std::vector<BaseType*> Entity::GetComponentsDerivedFrom() const
{
  auto res = std::vector<BaseType*>();

  for (vector<Component*>::const_iterator it = components.begin(); it != components.end(); ++it) {
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

template<typename ComponentType, typename... Args>
inline ComponentType* Entity::CreateComponent(Args&& ...args) {
  ComponentType* c = new ComponentType(std::forward<decltype(args)>(args)...);

  RegisterComponent(c);

  return c;
}