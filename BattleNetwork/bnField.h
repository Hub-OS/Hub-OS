#pragma once
#include <vector>
#include <map>
using std::map;
using std::vector;

#include "bindings/bnScriptedArtifact.h"
#include "bindings/bnScriptedSpell.h"
#include "bindings/bnScriptedObstacle.h"
#include "bnEntity.h"
#include "bnCharacterDeletePublisher.h"
#include "bnCharacterSpawnPublisher.h"

class Character;
class Spell;
class Obstacle;
class Artifact;
class Scene;

namespace Battle {
  class Tile;
}

#define BN_MAX_COMBAT_EVALUATION_STEPS 20

class Field : public std::enable_shared_from_this<Field>, public CharacterDeletePublisher, public CharacterSpawnPublisher{
public:
  using NotifyID_t = long long; // for lifetime notifiers

  friend class Entity;

  enum class AddEntityStatus {
    queued,
    added,
    deleted
  };

  /**
   * @brief Creates a field _wdith x _height tiles. Sets isTimeFrozen to false
   */
  Field(int _width, int _height);
  
  /**
   * @brief Deletes tiles
   */
  ~Field();

  /**
   * @brief sets the scene to this field for event emitters
   */
  void SetScene(const Scene* scene);

  /**
   * @brief Get the columns
   * @return columns
   */
  int GetWidth() const;
  
  /**
   * @brief Get the rows
   * @return rows
   */
  int GetHeight() const;

  NotifyID_t CallbackOnDelete(
    Entity::ID_t target,
    const std::function<void(std::shared_ptr<Entity>)>& callback
  );

  NotifyID_t NotifyOnDelete(
    Entity::ID_t target,
    Entity::ID_t observer,
    const std::function<void(std::shared_ptr<Entity>, std::shared_ptr<Entity>)>& callback
  );

  void DropNotifier(NotifyID_t notifier);

  /**
   * @brief Query for tiles that pass the input function
   * @param query input function, returns true or false based on conditions
   * @return vector of Tile* passing the input
   */
  std::vector<Battle::Tile*> FindTiles(std::function<bool(Battle::Tile* t)> query);

  /**
   * @brief Adds an entity
   * @param entity
   * @param x col
   * @param y row
   */
  AddEntityStatus AddEntity(std::shared_ptr<Entity> entity, int x, int y);
  AddEntityStatus AddEntity(std::shared_ptr<Entity> entity, Battle::Tile& dest);

  /**
   * @brief Callback for entities on the entire field
   * @param callback the callback function
   */
  void ForEachEntity(const std::function<void(std::shared_ptr<Entity>& e)>& callback);

  /**
   * @brief Callback for characters on the entire field
   * @param callback the callback function
   */
  void ForEachCharacter(const std::function<void(std::shared_ptr<Character>& e)>& callback);

  /**
   * @brief Callback for obstacles on the entire field
   * @param callback the callback function
   */
  void ForEachObstacle(const std::function<void(std::shared_ptr<Obstacle>& e)>& callback);

  /**
   * @brief Query for entities on the entire field
   * @param query. the query input function
   * @return list of std::shared_ptr<Entity> that passed the input function's conditions
   */
  std::vector<std::shared_ptr<Entity>> FindHittableEntities(std::function<bool(std::shared_ptr<Entity>& e)> query) const;

  /**
   * @brief Query for characters on the entire field
   * @param query. the query input function
   * @return list of std::shared_ptr<Character> that passed the input function's conditions
   */
  std::vector<std::shared_ptr<Character>> FindHittableCharacters(std::function<bool(std::shared_ptr<Character>& e)> query) const;

  /**
   * @brief Query for obstacles on the entire field
   * @param query. the query input function
   * @return list of std::shared_ptr<Obstacle> that passed the input function's conditions
   */
  std::vector<std::shared_ptr<Obstacle>> FindHittableObstacles(std::function<bool(std::shared_ptr<Obstacle>& e)> query) const;

  /**
   * @brief Query for the closest characters on the entire field given an input entity.
   * @param test. The entity to test distance against.
   * @param query. the query input function
   * @return list of std::shared_ptr<Character> that passed the input function's conditions
   */
  std::vector<std::shared_ptr<Character>> FindNearestCharacters(const std::shared_ptr<Entity> test, std::function<bool(std::shared_ptr<Character>& e)> query) const;

  /**
   * @brief Set the tile at (x,y) team to _team
   * @param _x
   * @param _y
   * @param _team
   */
  void SetAt(int _x, int _y, Team _team);
  
  /**
   * @brief Get the tile at (x,y)
   * @param _x col
   * @param _y row
   * @return null if x < 0 or x > 7 or y < 0 or y > 4, otherwise returns Tile*
   */
  Battle::Tile* GetAt(int _x, int _y) const;

  /**
   * @brief Updates all tiles
   * @param _elapsed in seconds
   */
  void Update(double _elapsed);
  
  /**
   * @brief Propagates the state to all tiles for specific behavior
   * @param state whether or not the battle is ongoing
   */
  void ToggleTimeFreeze(bool state);

  /**
 * @brief Propagates the state to all tiles for specific behavior
 */
  void RequestBattleStart();

  /**
* @brief Propagates the state to all tiles for specific behavior
*/
  void RequestBattleStop();

  /**
  * @brief Removes any pending entities that have not been added back to the field 
  * @param pointer to the tile 
  * @param ID long ID references the entity
  */
  void TileRequestsRemovalOfQueued(Battle::Tile*, Entity::ID_t ID);

  /**
  * @brief Entities will be added to the field during battle such as spells
  * 
  * In attempt to be frame-accurate to the original source material, all 
  * entities added on some frame N need to be spawned at the end of that frame
  */
  void SpawnPendingEntities();

  /**
  * @brief Returns true if pending.size() > 0
  *
  * Used when performing combat evalulation iterations at the end of the frame's update loop
  */
  const bool HasPendingEntities() const;

  /**
  * @brief Prevent duplicate update calls for a given entity during one update tick
  *
  * Because entities update on a per-tile basis, we need to ensure an entity spanning multiple tiles
  * Is only ever updated once.
  */
  void UpdateEntityOnce(Entity& entity, const double elapsed);

  /**
  * @brief removes the ID from allEntityHash
  */
  void ForgetEntity(Entity::ID_t ID);

  /**
  * @brief removes the ID from allEntityHash, safely removes from tiles, and deletes the entity pointer
  */
  void DeallocEntity(Entity::ID_t ID);

  /**
  * @brief returns the entity from the allEntityHash otherwise nullptr
  */
  std::shared_ptr<Entity> GetEntity(Entity::ID_t ID);

  /**
  * @brief returns the entity from the allEntityHash otherwise nullptr 
  */
  std::shared_ptr<Character> GetCharacter(Entity::ID_t ID);

  void RevealCounterFrames(bool enabled);

  const bool DoesRevealCounterFrames() const;

  void ClearAllReservations(Entity::ID_t ID);

  /**
  * @brief provides a default field arrangement if none are provided
  */
  void HandleMissingLayout();
private:
  bool isTimeFrozen; 
  bool isBattleActive; /*!< State flag if battle is active */
  bool revealCounterFrames; /*!< Adds color to enemies who can be countered*/
  int width; /*!< col */
  int height; /*!< rows */
  bool isUpdating; /*!< enqueue entities if added in the update loop */
  const Scene* scene{ nullptr };

  // Since we don't want to invalidate our entity lists while updating,
  // we create a pending queue of entities and tag them by type so later
  // we can add them into play
  struct queueBucket {
    int x{};
    int y{};
    Entity::ID_t ID{};
    std::shared_ptr<Entity> entity;

    queueBucket(int x, int y, std::shared_ptr<Entity> d);
    queueBucket(const queueBucket& rhs) = default;
  };

  struct DeleteObserver {
    NotifyID_t ID{};
    std::optional<Entity::ID_t> observer;
    std::function<void(std::shared_ptr<Entity>)> callback1; // target only variant
    std::function<void(std::shared_ptr<Entity>, std::shared_ptr<Entity>)> callback2; // target-observer variant
  };

  NotifyID_t nextID{};

  map<Entity::ID_t, std::shared_ptr<Entity>> allEntityHash; /*!< Quick lookup of entities on the field */
  map<Entity::ID_t, void*> updatedEntities; /*!< Since entities can be shared across tiles, prevent multiple updates*/
  map<Entity::ID_t, std::vector<DeleteObserver>> entityDeleteObservers; /*!< List of callback functions for when an entity is deleted*/
  map<NotifyID_t, Entity::ID_t> notify2TargetHash; /*!< Convert from target entity to its delete observer key*/
  vector<queueBucket> pending;
  vector<vector<Battle::Tile*>> tiles; /*!< Nested vector to make calls via tiles[x][y] */
};