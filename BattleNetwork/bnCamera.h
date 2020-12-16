#pragma once
#include <SFML/Graphics.hpp>

/**
 * @class Camera
 * @author mav
 * @date 05/05/19
 * @brief Camera can move over time and shake
 */
class Camera
{
sf::Vector2f dest; /*!< Camera destination if moving */
sf::Vector2f origin; /*!< Camera' origin */
sf::Time dur; /*!< Duration of movement */
sf::Time shakeDur; /*!< Duration of shake effect */
double stress; /*!< How much stress to apply to shake */
sf::View focus; /*!< SFML view of camera center */
sf::View init; /*!< Initial view */
float progress; /*!< Progress of movement */
float shakeProgress; /*!< Progress of shake effect */
bool isShaking; /*!< Flag for shaking camera */

public:
  /**
    * @brief Constructs a camera from a view */
  Camera(const sf::View& view);
  void operator=(const Camera& rhs); // copy ctor
  ~Camera();

  /**
   * @brief Moves camera to destination and shakes camera if applicable
   * @param elapsed in seconds
   */
  void Update(double elapsed);
  
  /**
   * @brief Move camera linearly to destination over time
   * @param destination point in world space
   * @param duration time
   */
  void MoveCamera(sf::Vector2f destination, sf::Time duration);
  
  /**
   * @brief Teleport camera to position
   * @param pos point in world space
   */
  void PlaceCamera(sf::Vector2f pos);
  
  /**
   * @brief Translate camera from current position
   * @param offset speed in world space
   */
  void OffsetCamera(sf::Vector2f offset);
  
  /**
   * @brief Query if a sprite is in view by the camera
   * @param sprite
   * @return true if in view, otherwise the sprite is off-screen
   */
  bool IsInView(sf::Sprite & sprite);
  
  /**
   * @brief Begins shake effect
   * @param stress intensity of the camera effect
   * @param duration how long shake should last
   * 
   * If a weaker shake requests, it is ignored.
   * There must be no shaking or a stronger shake to activate.
   */
  void ShakeCamera(double stress, sf::Time duration);
  
  /**
   * @brief Return an SFML view to use to show what camera sees
   * @return const sf::View
   */
  const sf::View GetView() const;
};

