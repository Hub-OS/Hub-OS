/*! \brief Tiles represent where entities can move around on.
 * 
 * Without a tile, an entity cannot exist in the battle field.
 * Tiles have states such as LAVA, ICE, GRASS, NORMAL, CRACKED, BROKEN, and EMPTY.
 * Each state may affect an entity that lands on it or prevent other entities 
 * from occupying them.
 * 
 * The tiles are responsible for updating everything that exists on it. To achieve this
 * each type of entity has divided into different buckets: artifacts, spells, characters, 
 * and obstacles. Spells can affect characters and obstacles can affect both spells and 
 * other characters while artifacts are seen as purely visual and don't affect others.
 * 
 * When entities move they adopt new tiles and should remove themselves from their
 * previous tile. The tile will remove the entity from its appropriate bucket.
 */

#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <set>
#include <algorithm>
#include <functional>
using sf::RectangleShape;
using sf::Sprite;
using std::vector;
using std::find;
using std::set;

class Entity;
class Spell;
class Character;
class Obstacle;
class Artifact;
class Field;

#include "bnTeam.h"
#include "bnTextureType.h"
#include "bnTileState.h"

namespace Battle {
  class Tile : public Sprite {
  public:
    friend class Entity;

    /**
    * \brief Base 1. Creates a tile at column x and row y.
    * 
    * If the tile column is <= 3, the tile's team is
    * automatically RED. Otherwise it is BLUE.
    */
    Tile(int _x, int _y);
    ~Tile();

    Tile(const Tile& rhs);
    Tile& operator=(const Tile& other);

    /**
     * @brief Query the current state of the tile
     * @return TileState
     */
    const TileState GetState() const;

    /**
     * @brief Assigns the tile's field pointer
     * @param _field
     */
    void SetField(Field* _field);

    /**
     * @brief Base 1. Returns the column of the tile.
     * @return Column number
     */
    const int GetX() const;
    
    /**
     * @brief Base 1. Returns the row of the tile.
     * @return Row number
     */
    const int GetY() const;

    /**
     * @brief Query the tile's team
     * @return Team
     */
    const Team GetTeam() const;
    
    /**
     * @brief Change the tile's team if unoccupied
     * This will also change the color of the tile. 
     * If the tile has characters of another team on it,
     * it will not change. 
     * @param _team Team to change to
     */
    void SetTeam(Team _team);

    /**
     * @brief Get the width of the tile sprite
     * @return width in pixels
     */
    float GetWidth() const;

    /**
     * @brief Get the number of entities occupying this tile
     * Size
     */
    const size_t GetEntityCount() const { return this->entities.size(); }
    
    /**
     * @brief Get the height of the tile sprite
     * @return height in pixels
     */
    float GetHeight() const;

    /**
     * @brief Change the tile's state if unoccupied or not broken nor empty
     * SetState tries to abide by the game's original rules
     * You cannot use a chip to change the state of the tile if it's in a state
     * that cannot be modified such as being TileState::Empty
     * @param _state
     */
    void SetState(TileState _state);

    /**
     * @brief Texture updates to reflect the tile's current state
     */
    void RefreshTexture();

    /**
     * @brief Query if the tile is walkable e.g. not Broken nor Empty
     * @return true if walkable, false otherwise
     */
    bool IsWalkable() const;
    
    /**
     * @brief Query if the tile is cracked
     * @return true if state == TileState::CRACKED 
     */
    bool IsCracked() const;

    /**
     * @brief Query if the tile should be highlighted in battle
     * Some spells don't highlight tiles they occupy
     * The boolean is toggled in the Tile's Update() loop 
     * In the battle scene, if a tile is highlighted, it will use the
     * yellow luminocity shader to highlight the tile.
     * @return true if tile should be highlighted
     */
    bool IsHighlighted() const;

    /**
     * @brief Adds a spell to the spell bucket if it doesn't already exist
     * @param _entity
     */
    void AddEntity(Spell& _entity);
    
    /**
     * @brief Adds a character to the character bucket if it doesn't already exist
     * @param _entity
     */
    void AddEntity(Character& _entity);
    
    /**
     * @brief Adds an obstacle to the obstacle bucket if it doesn't already exist
     * @param _entity
     */
    void AddEntity(Obstacle& _entity);
    
    /**
     * @brief Adds an artifact to the artifact bucket if it doesn't already exist
     * @param _entity
     */
    void AddEntity(Artifact& _entity);

    /**
     * @brief If found, remove the entity from all buckets with the same ID
     * @param ID
     */
    void RemoveEntityByID(int ID);
    
    /**
     * @brief Query if the given entity already occupies this tile
     * @param _entity to check
     * @return true if the entity is already in one of the buckets
     */
    bool ContainsEntity(Entity* _entity) const;

    /**
     * @brief Reserve this tile for an entity with ID 
     * An entity may be temporarily removed from play but will be back
     * later. This mechanic is seen with AntiDamage chip. While
     * the entity is removed, the tile it was on must not be modified
     * by state changes and should be treated as occupied for all other checks.
     * When the entity is added again with AddEntity(), the reserve will be removed.
     * @param ID
     */
    void ReserveEntityByID(int ID);

    /**
     * @brief Query the tile for an entity type
     * @return true if the tile contains entity of type Type, false if no matches
     */
    template<class Type> bool ContainsEntityType();
    
    /**
     * @brief Attack all entities occupying this tile with spell 
     * @param caller must be valid non-null spell
     */
    void AffectEntities(Spell* caller);

    /**
     * @brief Updates all entities occupying this tile
     * @param _elapsed in seconds
     */
    void Update(float _elapsed);

    /**
     * @brief Notifies all entities that the battle is active
     * @param state
     */
    void SetBattleActive(bool state);

    /**
     * @brief Query for multiple entities using a functor
     * This is useful for movement as well as chip attacks 
     * to find specific enemy types under specific conditions
     * @param e Functor that takes in an entity and returns a boolean
     * @return returns a list of entities that returned true in the functor `e` 
     */
    std::vector<Entity*> FindEntities(std::function<bool(Entity*e)> query);

  private:
    int x; /**< Column number*/
    int y; /**< Row number*/
    Team team;
    TileState state;
    TextureType textureType; /**< Texture reflects the tile's state */
    float elapsed; /**< Internal counter for non-permanent states e.g. TileState::Cracked */

    float width;
    float height;
    Field* field;
    float cooldown;
    float cooldownLength;
    bool hasSpell; /**< Highlights when there is a spell occupied in this tile */
    bool isBattleActive;

    // Todo: use sets to avoid duplicate entries
    vector<Artifact*> artifacts; /**< Entity bucket for type Artifacts */
    vector<Spell*> spells; /**< Entity bucket for type Spells */
    vector<Character*> characters; /**< Entity bucket for type Characters */
    vector<Entity*> entities; /**< Entity bucket for looping over all entities **/

    set<int> reserved; /**< IDs of entities reserving this tile*/
    vector<long> taggedSpells; /**< IDs of occupying spells that have already attacked this frame*/

    /**
     * @brief Auxillary function used by all other overloads of AddEntity
     * @param _entity
     */
    void AddEntity(Entity* _entity);
  };


  template<class Type>
  bool Tile::ContainsEntityType() {
    // std::cout << "len of entities is: " << entities.size() << "\n";

    for (vector<Entity*>::iterator it = entities.begin(); it != entities.end(); ++it) {
      if (dynamic_cast<Type*>(*it) != nullptr) {
        return true;
      }
    }

    return false;
  }
}