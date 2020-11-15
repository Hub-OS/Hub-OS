#pragma once
#include "bnArtifact.h"
#include "bnComponent.h"
#include "bnField.h"
#include "bnDefenseBubbleWrap.h"
#include "bnInputManager.h"
#include <vector>

/**
 * @class BubbleTrap
 * @author mav
 * @date 05/05/19
 * @brief Visual artifact (bubble animates) that can be attached to an entity
 * 
 * Creates a defense rule to absord first damage and forces this bubble to pop
 * On pop, frees owner and deletes self
 */
class BubbleTrap : public SpriteProxyNode, public Component
{
private:
  Animation animation;
  sf::Sprite bubble;
  double duration; /*!< when this reaches zero, pops */
  DefenseBubbleWrap* defense; /*!< Add BubbleWrapTrap defense rule */
  bool willDelete;
  std::vector<InputEvent> lastFrameStates;
public:
  /**
   * @brief Attaches to the owner, sets the animation */
  BubbleTrap(Character* owner);
  
  ~BubbleTrap();
  
  /**
   * @brief Does not inject into the battle scene
   * 
   */
  void Inject(BattleSceneBase&) override;
  
  /**
   * @brief Animators bubble. When timer runs out, pops the bubble
   * @param _elapsed in seconds
   */
  void OnUpdate(float _elapsed) override;

  /**
   * @brief Sets the pop animation and deletes self when over
   */
  void Pop();

  const double GetDuration() const;
};
