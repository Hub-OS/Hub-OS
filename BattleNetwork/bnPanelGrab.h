
/*! \brief Orb drops down and changes tile team */

#pragma once
#include "bnSpell.h"
#include "bnTile.h"
#include "bnAnimationComponent.h"

class PanelGrab : public Spell {
private:
  sf::Vector2f start; /*!< Where the orb starts */
  double progress;    /*!< Progress from the start to the tile */
  double duration;    /*!< How long the animation should last in seconds */
  AnimationComponent* animationComponent{ nullptr };
public:
  /**
   * @brief sets the team it will change the tile to and duration of animation 
   * @param _field field to add to
   * @param _team team it will change tile to
   * @param _duration length of the animation
   */
  PanelGrab(Team _team,float _duration);
  
  /**
   * @brief deconstructor
   */
  ~PanelGrab();

  /**
  * @brief Update the sprite position 
  * @param start The tile to spawn onto
  */
  void OnSpawn(Battle::Tile& start) override;

  /**
   * @brief Interpolate from start pos to tile and changes tile team
   * @param _elapsed in seconds
   */
  void OnUpdate(double _elapsed) override;
  
  /**
   * @brief PanelGrab does not move across field
   * @param _direction ignored
   * @return false
   */
  bool Move(Direction _direction) override;
  
  /**
   * @brief Deals 10 damage
   * @param _entity to hit
   */
  void Attack(Character* _entity) override;

  /** 
  * @brief Does nothing
  */
  void OnDelete() override;
};