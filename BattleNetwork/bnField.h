#pragma once
#include <vector>
#include <map>
using std::map;
using std::vector;

#include "bnEntity.h"
#include "bnCharacterDeletePublisher.h"

class Character;
class Spell;
class Obstacle;
class Artifact;

namespace Battle {
  class Tile;
}

#define BN_MAX_COMBAT_EVALUATION_STEPS 20

class Field : public CharacterDeletePublisher{
public:
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
   * @brief Get the columns
   * @return columns
   */
  int GetWidth() const;
  
  /**
   * @brief Get the rows
   * @return rows
   */
  int GetHeight() const;

  /**
   * @brief Query for tiles that pass the input function
   * @param query input function, returns true or false based on conditions
   * @return vector of Tile* passing the input
   */
  std::vector<Battle::Tile*> FindTiles(std::function<bool(Battle::Tile* t)> query);

  /**
   * @brief Adds a character using the character's AdoptTile() routine
   * @param character
   * @param x col
   * @param y row
   */
  AddEntityStatus AddEntity(Character& character, int x, int y);
  AddEntityStatus AddEntity(Character& character, Battle::Tile& dest);

  /**
   * @brief Adds a spell using the spell's AdoptTile() routine
   * @param spell
   * @param x col
   * @param y row
   */
  AddEntityStatus AddEntity(Spell& spell, int x, int y);
  AddEntityStatus AddEntity(Spell& spell, Battle::Tile& dest);

  /**
   * @brief Adds an obstacle using the obstacle's AdoptTile() routine
   * @param obst
   * @param x col
   * @param y row
   */
  AddEntityStatus AddEntity(Obstacle& obst, int x, int y);
  AddEntityStatus AddEntity(Obstacle& obst, Battle::Tile& dest);

  /**
   * @brief Adds an artifact using the artifact's AdoptTile() routine
   * @param art
   * @param x col
   * @param y row
   */
  AddEntityStatus AddEntity(Artifact& art, int x, int y);
  AddEntityStatus AddEntity(Artifact& art, Battle::Tile& dest);

  /**
   * @brief Query for entities on the entire field
   * @param e the query input function
   * @return list of Entity* that passed the input function's conditions
   */
  std::vector<Entity*> FindEntities(std::function<bool(Entity* e)> query);

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
   * @return null if x < 1 or x > 6 or y < 1 or y > 3, otherwise returns Tile*
   */
  Battle::Tile* GetAt(int _x, int _y) const;

  /**
   * @brief Updates all tiles
   * @param _elapsed in seconds
   */
  void Update(float _elapsed);
  
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
  void UpdateEntityOnce(Entity* entity, const float elapsed);

  /**
  * @brief removes the ID from allEntityHash
  */
  void ForgetEntity(Entity::ID_t ID);

  /**
  * @brief returns the entity from the allEntityHash otherwise nullptr
  */
  Entity* GetEntity(Entity::ID_t ID);

private:

  bool isTimeFrozen; 
  bool isBattleActive; /*!< State flag if battle is active */
  int width; /*!< col */
  int height; /*!< rows */
  bool isUpdating; /*!< enqueue entities if added in the update loop */

  // Since we don't want to invalidate our entity lists while updating,
  // we create a pending queue of entities and tag them by type so later
  // we can add them into play
  struct queueBucket {
    int x;
    int y;
    Entity::ID_t ID;

    enum class type : int {
      character,
      spell,
      obstacle,
      artifact
    } entity_type;

    union type_data {
      Character* character;
      Spell* spell;
      Obstacle* obstacle;
      Artifact* artifact;
    } data;

    queueBucket(int x, int y, Character& d);
    queueBucket(int x, int y, Obstacle& d);
    queueBucket(int x, int y, Artifact& d);
    queueBucket(int x, int y, Spell& d);
    queueBucket(const queueBucket& rhs) = default;
  };
  map<Entity::ID_t, Entity*> allEntityHash; /*!< Quick lookup of entities on the field */
  map<Entity::ID_t, void*> updatedEntities; /*!< Since entities can be shared across tiles, prevent multiple updates*/
  vector<queueBucket> pending;
  vector<vector<Battle::Tile*>> tiles; /*!< Nested vector to make calls via tiles[x][y] */
};