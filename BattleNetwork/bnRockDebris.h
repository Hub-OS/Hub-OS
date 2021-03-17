/*! \brief When Cubes break, two rock pieces emit for effect */
#pragma once
#include "bnArtifact.h"
#include "bnField.h"

class RockDebris : public Artifact
{
public:
  /*! \class RockDebris::Type 
   *  \brief Deterimines which sprites to use
   */
  enum class Type : int {
    LEFT = 1,
    RIGHT,
    LEFT_ICE,
    RIGHT_ICE
  };

private:
  Animation animation; /*!< Rocks do not animate but use the same spritesheet */

  sf::Sprite leftRock; /*!< Left rock sprite */
  sf::Sprite rightRock; /*!< Right rock sprite */

  double duration; /*!< The total duration this effect last */
  double progress; /*!< How far into the effect */
  double intensity; /*!< How far upwards, out, and how fast the effect is */
  Type type; /*!< Which piece this is */
public:
  /**
   * @brief Sets the correct frame in the spritesheet based on the type 
   * @param intensity How intense the effect is
   */
  RockDebris(Type type, double intensity);
  
  /**
   * @brief deconstructor
   */
  ~RockDebris();

  /**
   * @brief Uses separate x and y interpolation to look like there's force and gravity
   * @param _elapsed in seconds
   */
  void OnUpdate(double _elapsed) override;

  /**
  * @brief Removes debris from play
  */
  void OnDelete() override;
};

