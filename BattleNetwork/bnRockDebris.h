<<<<<<< HEAD
=======
/*! \file bnRockDebris.h */

/*! \brief When Cubes break, two rock pieces emit for effect */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
#pragma once
#include "bnArtifact.h"
#include "bnField.h"

class RockDebris : public Artifact
{
public:
<<<<<<< HEAD
=======
  /*! \class RockDebris::Type 
   *  \brief Deterimines which sprites to use
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  enum class Type : int {
    LEFT = 1,
    RIGHT,
    LEFT_ICE,
    RIGHT_ICE
  };

private:
<<<<<<< HEAD
  Animation animation;

  sf::Sprite leftRock;
  sf::Sprite rightRock;

  double duration;
  double progress;
  double intensity;
  Type type;
public:
  RockDebris(Type type, double intensity);
  ~RockDebris();

  virtual void Update(float _elapsed);
=======
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
  virtual void Update(float _elapsed);
  
  /**
   * @brief Does not move along the grid
   * @param _direction ignored
   * @return false
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  virtual bool Move(Direction _direction) { return false; }
};

