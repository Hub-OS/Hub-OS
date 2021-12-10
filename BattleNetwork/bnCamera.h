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
private:
  bool isShaking{ false }; /*!< Flag for shaking camera */
  bool waning{ false }; /*!< Toggle how movement is interpolated */
  double progress{}; /*!< Progress of movement */
  double shakeProgress{}; /*!< Progress of shake effect */
  double stress{}; /*!< How much stress to apply to shake */
  double waneFactor{};
  double fadeProgress{}; /*!< progress in fade effect*/
  sf::Vector2f dest{}; /*!< Camera destination if moving */
  sf::Vector2f origin{}; /*!< Camera' origin */
  sf::Time dur; /*!< Duration of movement */
  sf::Time shakeDur; /*!< Duration of shake effect */
  sf::View focus; /*!< SFML view of camera center */
  sf::View init; /*!< Initial view */
  sf::Color fadeColor{}; /*!< Color of the fade*/
  sf::Color startColor{}; /*!< starting Color of the fade*/
  sf::Time fadeDur; /*! duration of the fade effect*/
  sf::RectangleShape rect; /*! Covers the screen */
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
   * @brief Burst of movement and then inches toward destination
   * @param destination point in world space
   * @param duration time
   * @param factor waning factor. smaller (0.01 - 0.5) is snappier while (0.5-1.0) is smoother. anything past f > 1.0 becomes linear at the cost of a delay.
   */
  void WaneCamera(sf::Vector2f destination, sf::Time duration, float factor);

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
  const sf::View& GetView() const;

  /**
   * @brief Cinematic fade out controlled by the camera
   * @param color. Color to fade out or in with
   * @param time. The duration of the effect.
   */
  void FadeCamera(const sf::Color& color, sf::Time duration);

  const sf::RectangleShape& GetLens();
};

