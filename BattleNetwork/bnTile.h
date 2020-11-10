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
class Dummy;

#include "bnTeam.h"
#include "bnTextureType.h"
#include "bnTileState.h"
#include "bnAnimation.h"
#include "bnField.h"
#include "bnDefenseRule.h"

namespace Battle {
  class Tile : public SpriteProxyNode {
  public:
    enum class Highlight : int {
      none = 0,
      flash = 1,
      solid = 2,
    };

    friend Field::Field(int _width, int _height);
    friend void Field::Update(float _elapsed);

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
     * @param useFlicker if true, will change state & will flicker for flickerTeamCooldownLength milliseconds
     */
    void SetTeam(Team _team, bool useFlicker = false);

    /**
     * @brief Get the width of the tile sprite
     * @return width in pixels
     */
    float GetWidth() const;

    /**
     * @brief Get the number of entities occupying this tile
     * Size
     */
    const size_t GetEntityCount() const { return entities.size(); }
    
    /**
     * @brief Get the height of the tile sprite
     * @return height in pixels
     */
    float GetHeight() const;

    /**
     * @brief Change the tile's state if unoccupied or not broken nor empty
     * SetState tries to abide by the game's original rules
     * You cannot use a card to change the state of the tile if it's in a state
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
     * @return true if state == TileState::cracked 
     */
    bool IsCracked() const;

    /**
   * @brief Query if the tile is an edge tile
   * @return true if x = {0, 7} or y = {0, 4}
   */
    bool IsEdgeTile() const;

    /**
     * @brief Query if the tile should be highlighted in battle
     * Some spells don't highlight tiles they occupy
     * The boolean is toggled in the Tile's Update() loop 
     * In the battle scene, if a tile is highlighted, it will use the
     * yellow luminocity shader to highlight the tile.
     * @return true if tile will be highlighted
     */
    bool IsHighlighted() const;

    /**
     * @brief will request a highlight style for one frame
     */
    void RequestHighlight(Highlight mode);

    /**
     * @brief Returns true if a character is standing on or has reserved this tile 
     * 
     * Only checks for character entity types
     */
    bool IsReservedByCharacter(); 

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
     * @return true if the entity bucket has been modified and the entity removed
     */
    bool RemoveEntityByID(Entity::ID_t ID);
    
    /**
     * @brief Query if the given entity already occupies this tile
     * @param _entity to check
     * @return true if the entity is already in one of the buckets
     */
    bool ContainsEntity(const Entity* _entity) const;

    /**
     * @brief Reserve this tile for an entity with ID 
     * An entity may be temporarily removed from play but will be back
     * later. This mechanic is seen with AntiDamage card. While
     * the entity is removed, the tile it was on must not be modified
     * by state changes and should be treated as occupied for all other checks.
     * When the entity is added again with AddEntity(), the reserve will be removed.
     * @param ID
     */
    void ReserveEntityByID(long ID);

    /**
     * @brief Query the tile for an entity type
     * @return true if the tile contains entity of type Type, false if no matches
     */
    template<class Type> bool ContainsEntityType();
    
    /**
     * @brief Queues spell to all entities occupying this tile with spell 
     * @param caller must be valid non-null spell
     */
    void AffectEntities(Spell* caller);

    /**
     * @brief Updates all entities occupying this tile
     * @param _elapsed in seconds
     */
    void Update(float _elapsed);

    /**
     * @brief Triggers this tile and all entities to behave as if time is frozen
     * @param state
     */
    void ToggleTimeFreeze(bool state);

    /**
     * @brief Notifies all entities that the battle is active
     */
    void BattleStart();

    /**
     * @brief Notifies all entities that the battle is inactive
     */
    void BattleStop();

    void HandleTileBehaviors(Obstacle * obst);
    void HandleTileBehaviors(Character* character);

    /**
     * @brief Query for multiple entities using a functor
     * This is useful for movement as well as card attacks 
     * to find specific enemy types under specific conditions
     * @param e Functor that takes in an entity and returns a boolean
     * @return returns a list of entities that returned true in the functor `e` 
     */
    std::vector<Entity*> FindEntities(std::function<bool(Entity*e)> query);

  private:

    std::string GetAnimState(const TileState state);

    void CleanupEntities();
    void ExecuteAllSpellAttacks();
    void UpdateSpells(const float elapsed);
    void UpdateArtifacts(const float elapsed);
    void UpdateCharacters(const float elapsed);

    int x; /**< Column number*/
    int y; /**< Row number*/
    Team team;
    TileState state;
    std::string animState; /**< reflects the tile's state - lookup animation from animation file */

    float width;
    float height;
    Field* field;
    float teamCooldown;

    std::shared_ptr<sf::Texture> red_team_atlas;
    std::shared_ptr<sf::Texture> blue_team_atlas;

    static float teamCooldownLength;
    float brokenCooldown;
    static float brokenCooldownLength;
    float flickerTeamCooldown;
    static float flickerTeamCooldownLength;
    float totalElapsed;
    bool willHighlight; /**< Highlights when there is a spell occupied in this tile */
    Highlight highlightMode;
    bool isTimeFrozen;
    bool isBattleOver;
    bool isBattleStarted{ false };
    double elapsedBurnTime;
    double burncycle;

    // Todo: use sets to avoid duplicate entries
    vector<Artifact*> artifacts; /**< Entity bucket for type Artifacts */
    vector<Spell*> spells; /**< Entity bucket for type Spells */
    vector<Character*> characters; /**< Entity bucket for type Characters */

    set<Character*, EntityComparitor> deletingCharacters;

    vector<Entity*> entities; /**< Entity bucket for looping over all entities **/

    set<Entity::ID_t> reserved; /**< IDs of entities reserving this tile*/
    vector<Entity::ID_t> queuedSpells; /**< IDs of occupying spells that have signaled they are to attack this frame */
    vector<Entity::ID_t> taggedSpells; /**< IDs of occupying spells that have already attacked this frame*/

    Animation animation;
    Animation volcanoErupt;
    double volcanoEruptTimer{ 4 }; // seconds
    SpriteProxyNode volcanoSprite;

    /**
     * @brief Auxillary function used by all other overloads of AddEntity
     * @param _entity
     */
    void AddEntity(Entity* _entity);
  };

  template<class Type>
  bool Tile::ContainsEntityType() {
    for (vector<Entity*>::iterator it = entities.begin(); it != entities.end(); ++it) {
      if (dynamic_cast<Type*>(*it) != nullptr) {
        return true;
      }
    }

    return false;
  }
}