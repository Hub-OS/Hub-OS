#pragma once
#include "bnArtifact.h"
#include "bnAnimationComponent.h"

/**
 * @class Explosion
 * @author mav
 * @date 04/05/19
 * @brief Explosion spawns as many explosions over time if the count is > 1
 * 
 * First explosion is always centered on the tile and on the top-most layer
 * 
 * If the number of explosions is > 1 the following explosions will be set on the
 * bottom layer and offset by a random amount to recreate the effect seen in the game
 */
class Explosion : public Artifact
{
private:
  AnimationComponent* animationComponent; /*!< Animator the explosion */
  int numOfExplosions; /*!< Once the count reaches this number, the effect is over */
  sf::Vector2f offset{}; /*!< Explosion children are placed randomly around the spawn area */
  sf::Vector2f offsetArea{}; /*!< Screen space relative to origin to randomly pick from*/
  int count{ 0 }; /*!< Used by root to keep track of explosions left */
  Explosion* root; /*!< The explosion that starts the chain */
  double playbackSpeed; /*!< The speed of the explosion effect. Bosses have higher speeds */
  
   /** 
   * @brief Used internally by root explosions to create childen explosions
   * @param copy the root explosion to copy settings from
   */
  Explosion(const Explosion& copy);

public:
  /**
   * @brief Create an explosion chain effect with numOfExplosions=1 and playbackSpeed=0.55 defaults
   */
  Explosion(int _numOfExplosions=1, double _playbackSpeed=0.55);
  
  ~Explosion();

  /**
   * @brief If root increment count is size of numOfExplosions, delete and stop the chain 
   * @param _elapsed in seconds
   */
  void OnUpdate(double _elapsed) override;

  void OnDelete() override;
  
  void OnSpawn(Battle::Tile& start) override;

  /**
   * @brief Explosion doesnt move
   * @param _direction ignored
   * @return false
   */
  virtual bool Move(Direction _direction) override;

  /**
   * @brief Used by root. Increment the number of explosions
   */
  void IncrementExplosionCount();

  /**
 * @brief area.x is width, area.y is height relative to origin to explode in
 */
  void SetOffsetArea(sf::Vector2f area);
};

