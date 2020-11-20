#include "../bnTextureResourceManager.h"
#include "../bnSpriteProxyNode.h"
#include "../bnDirection.h"
#include "../bnAnimation.h"
#include "../bnEngine.h"

#include <map>

namespace Overworld {

  /**
    @brief Overworld::Actor class represents a character that can move, has animations for all movements, and has a name
  */

  class Actor : public SpriteProxyNode {
  public:
    enum class MovementState : unsigned char {
      idle = 0,
      walking,
      running,
      size = 3
    };
  private:
    double animProgress{}; //!< Used to sync movement animations
    double walkSpeed{40}; //!< walk speed as pixels per second. Default 40px/s
    double runSpeed{70}; //!< run speed as pixels per second. Default 70px/s
    Direction heading{ Direction::down }; //!< the character's current heading
    std::map<std::string, Animation> anims; //!< Map of animation objects per direction per state
    MovementState state{}; //!< Current movement state (idle, moving, or running)
    sf::Vector2f pos{}; //!< 2d position in cartesian coordinates
    std::string name{}; //!< name of this character
    std::string lastStateStr{}; //!< String representing the last frame's state name
    // TODO: collision

    // aux functions
    std::string DirectionAnimStrSuffix(const Direction& dir);
    std::string MovementAnimStrPrefix(const MovementState& state);
  public:
    /**
    * @brief Construct a character with a name
    */
    Actor(const std::string& name);

    Actor(const Actor&) = delete;

    /**
    * @brief Deconstructor
    */
    ~Actor();

    /**
    * @brief Make the character walk
    * @param dir direction to walk in
    */
    void Walk(const Direction& dir);

    /**
    * @brief Make the character run
    * @param dir direction to run in
    */
    void Run(const Direction& dir);

    /**
    * @brief Make the character idle and face a direction
    * @param dir direction to face. movement speed is applied as zero.
    */
    void Face(const Direction& dir);

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
    void LoadAnimations(const std::string& path);

    /**
    * @brief Set the walk speed as pixels per second
    * @param speed Pixels per second
    */
    void SetWalkSpeed(const double speed);

    /**
    * @brief Set the run speed as pixels per second
    * @param speed Pixels per second
    */
    void SetRunSpeed(const double speed);

    /**
    * @brief Fetch the name of this object
    * @return name of the character
    */
    const std::string GetName() const;

    /**
    * @brief Fetch the walking speed
    * @return walk speed as pixels per second
    */
    const double GetWalkSpeed() const;

    /**
    * @brief Fetch the running speed
    * @return run speed as pixels per second
    */
    const double GetRunSpeed() const;

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
    * @brief Update the actor location and frame
    * @param elapsed Time elapsed in seconds
    * 
    * This funtion will set the appropriate animation if it is not set to
    * reflect the current state values. It will also offset the actor x/y
    * based on walk or run speeds.
    */
    void Update(double elapsed);
  };
}