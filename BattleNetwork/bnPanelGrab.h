<<<<<<< HEAD
=======
/*! \brief Orb drops down and changes tile team */

>>>>>>> b486e21e11627262088deae73097eaa7af56791c
#pragma once
#include "bnSpell.h"
#include "bnTile.h"

class PanelGrab : public Spell {
private:
<<<<<<< HEAD
  sf::Vector2f start;
  double progress;
  double duration;

public:
  PanelGrab(Field* _field, Team _team, float _duration);
  virtual ~PanelGrab(void);

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction);
=======
  sf::Vector2f start; /*!< Where the orb starts */
  double progress;    /*!< Progress from the start to the tile */
  double duration;    /*!< How long the animation should last in seconds */

public:
  /**
   * @brief sets the team it will change the tile to and duration of animation 
   * @param _field field to add to
   * @param _team team it will change tile to
   * @param _duration length of the animation
   */
  PanelGrab(Field* _field, Team _team, float _duration);
  
  /**
   * @brief deconstructor
   */
  virtual ~PanelGrab();

  /**
   * @brief Interpolate from start pos to tile and changes tile team
   * @param _elapsed in seconds
   */
  virtual void Update(float _elapsed);
  
  /**
   * @brief PanelGrab does not move across field
   * @param _direction ignored
   * @return false
   */
  virtual bool Move(Direction _direction);
  
  /**
   * @brief Deals 10 damage
   * @param _entity to hit
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  virtual void Attack(Character* _entity);
};