#pragma once
#include <vector>

#include "bnSpriteProxyNode.h"
#include "bnAnimation.h"
#include "bnResourceHandle.h"

class ExplosionSpriteNode : public SpriteProxyNode, public ResourceHandle
{
private:
  SceneNode* parent{ nullptr };
  int numOfExplosions; /*!< Once the count reaches this number, the effect is over */
  sf::Vector2f offset{}; /*!< Explosion children are placed randomly around the spawn area */
  sf::Vector2f offsetArea{}; /*!< Screen space relative to origin to randomly pick from*/
  int count{ 0 }; /*!< Used by root to keep track of explosions left */
  ExplosionSpriteNode* root{ nullptr }; /*!< The explosion that starts the chain */
  
  std::vector<ExplosionSpriteNode*> chain;
  bool done{ false };
  
  double playbackSpeed; /*!< The speed of the explosion effect. Bosses have higher speeds */
  Animation animation;
   /** 
   * @brief Used internally by root explosions to create childen explosions
   * @param copy the root explosion to copy settings from
   */
  ExplosionSpriteNode(const ExplosionSpriteNode& copy);

public:
  /**
   * @brief Create an explosion chain effect with numOfExplosions=1 and playbackSpeed=0.55 defaults
   */
  ExplosionSpriteNode(SceneNode* parent, int _numOfExplosions=1, double _playbackSpeed=0.55);
  
  ~ExplosionSpriteNode();

  /**
   * @brief If root increment count is size of numOfExplosions, delete and stop the chain 
   * @param _elapsed in seconds
   */
  void Update(double _elapsed);

  /**
   * @brief Used by root. Increment the number of explosions
   */
  void IncrementExplosionCount();

  /**
 * @brief area.x is width, area.y is height relative to origin to explode in
 */
  void SetOffsetArea(sf::Vector2f area);

  const bool IsSequenceComplete() const;
  std::vector<ExplosionSpriteNode*> GetChain();
};

