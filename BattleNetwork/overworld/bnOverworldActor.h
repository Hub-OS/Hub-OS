#pragma once

#include "bnOverworldSprite.h"
#include "bnOverworldSpatialMap.h"
#include "../bnTextureResourceManager.h"
#include "../bnSpriteProxyNode.h"
#include "../bnDirection.h"
#include "../bnAnimation.h"
#include "../bnDrawWindow.h"

#include <map>
#include <optional>

namespace Overworld {
  class Map;
  class SpatialMap;

  /**
    @brief Overworld::Actor class represents a character that can move, has animations for all movements, and has a name
  */
  class Actor : public WorldSprite {
  public:
    enum class MovementState : unsigned char {
      idle = 0,
      walking,
      running,
      size = 3
    };
  private:
    struct AnimStatePair {
      MovementState movement{};
      Direction dir{};
    };

    Map* map{ nullptr };
    float animProgress{}; //!< Used to sync movement animations
    float walkSpeed{40}; //!< walk speed as pixels per second. Default 40px/s
    float runSpeed{70}; //!< run speed as pixels per second. Default 70px/s
    bool moveThisFrame{ false }; //!< Tells actor to move in accordance with their states or remain stationairy
    Direction heading{ Direction::down }; //!< the character's current heading
    std::map<std::string, Animation> anims; //!< Map of animation objects per direction per state
    std::vector<AnimStatePair> validStates; //!< Map of provided animations states
    MovementState state{}; //!< Current movement state (idle, moving, or running)
    sf::Vector2f pos{}; //!< 2d position in cartesian coordinates
    std::string name{}; //!< name of this character
    std::string lastStateStr{}; //!< String representing the last frame's state name
    std::function<void(std::shared_ptr<Actor> with)> onInteractFunc; //!< What happens if an actor interacts with the other
    float collisionRadius{ 1.0 };
    bool solid{true};

    void UpdateAnimationState(float elapsed);

    // aux functions
    std::string DirectionAnimStrSuffix(const Direction& dir);
    std::string MovementAnimStrPrefix(const MovementState& state);
    std::string FindValidAnimState(const Direction& dir, const MovementState& state);
  public:
    /**
    * @brief Construct a character with a name
    */
    Actor(const std::string& name);

    Actor(Actor&& ) noexcept;

    /**
    * @brief Deconstructor
    */
    ~Actor();

    /**
    * @brief Make the character walk
    * @param dir direction to walk in
    * @param move if true then move (default), if false remain in place
    */
    void Walk(const Direction& dir, bool move=true);

    /**
    * @brief Make the character run
    * @param dir direction to run in
    * @param move if true then move (default), if false remain in place
    */
    void Run(const Direction& dir, bool move=true);

    /**
    * @brief Make the character idle and face a direction
    * @param dir direction to face. movement speed is applied as zero.
    */
    void Face(const Direction& dir);

    /**
    * @brief Make the character idle and face a direction
    * @param dir direction to face. movement speed is applied as zero.
    */
    void Face(const Actor& actor);

    /**
    * @brief Loads animation data from a file
    * @param path load the animations for this character from a file
    * @preconditions The animation file follows the standard for overworld states described below
    * 
    * Each heading must end with a one letter suffix. For diagonal directions they are two letter suffixes.
    * For diagonal directions the y axis takes priority 
    *   e.g. _YX would be _DL for "Down Right"
    * 
    * Each movement state must prefix the heading with either "IDLE", "WALK", or "RUN"
    * e.g. "IDLE_L" would be "Idle left"
    *      "WALK_UL" would be "Walking Up left"
    *      "RUN_D" would be "Run Down"
    */
    void LoadAnimations(const Animation& animation);

    /**
    * @brief Set the walk speed as pixels per second
    * @param speed Pixels per second
    */
    void SetWalkSpeed(float speed);

    /**
    * @brief Set the run speed as pixels per second
    * @param speed Pixels per second
    */
    void SetRunSpeed(float speed);


    /**
    * @brief Replaces the name of the actor with a new name
    * @param newName of the actor
    */
    void Rename(const std::string& newName);

    /**
    * @brief Fetch the name of this object
    * @return name of the character
    */
    const std::string GetName() const;

    /**
    * @brief Fetch the walking speed
    * @return walk speed as pixels per second
    */
    float GetWalkSpeed() const;

    /**
    * @brief Fetch the running speed
    * @return run speed as pixels per second
    */
    float GetRunSpeed() const;

    /**
    * @brief Fetch the current animation state as a string
    * @return builds a string based off the movement and dir string aux functions
    */
    std::string CurrentAnimStr();

    /**
    * @brief Fetch the actor's heading
    * @return Direction
    */
    const Direction GetHeading() const;

    /**
    * @brief Return the position + heading by collisionRadius
    * @return vector in front of the actor's heading
    */
    sf::Vector2f PositionInFrontOf() const;

    /**
    * @brief Update the actor location and frame
    * @param elapsed Time elapsed in seconds
    * 
    * This funtion will set the appropriate animation if it is not set to
    * reflect the current state values. It will also offset the actor x/y
    * based on walk or run speeds.
    */
    void Update(float elapsed, SpatialMap& spatialMap);

    /**
    * @brief Watch for tile-based collisions with an existing map
    * @param map to collide with
    * 
    * During Update() if the actor is intended to move, it will also check
    * against tiles to see if it should collide.
    */
    void CollideWithMap(Map& map);

    /**
    * @brief Convert direction flags into 2D mathematical vector objects with a length
    * @param dir The direction to convert
    * @param length the unit value to set the vector according to the direction
    * @return vector to be used in geometrical calculations
    */
    static sf::Vector2f MakeVectorFromDirection(Direction dir, float length);

    /**
    * @brief Convert 2D mathematical vector objects to a Direction
    * @param vector to convert
    * @param threshold. If the vector is below this value, the direction is considered Direction::none
    * @return Direction to be used in motion and state cases
    */
    static Direction MakeDirectionFromVector(const sf::Vector2f& vec);

    void SetSolid(bool solid);
    float GetCollisionRadius();
    void SetCollisionRadius(float radius);
    void SetInteractCallback(const std::function<void(std::shared_ptr<Actor>)>& func);
    void Interact(const std::shared_ptr<Actor>& with);

    const std::optional<sf::Vector2f> CollidesWith(const Actor& actor, const sf::Vector2f& offset = sf::Vector2f{});
    const std::pair<bool, sf::Vector2f> CanMoveTo(Direction dir, MovementState state, float elapsed, SpatialMap& spatialMap);
  };
}
