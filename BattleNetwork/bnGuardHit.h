#pragma once
#pragma once
#include "bnArtifact.h"
#include "bnField.h"
#include "bnAnimationComponent.h"

/**
 * @class GuardHit
 * @author mav
 * @date 01/05/19
 * @brief Green _dink_ effect when guard is active
 */
class GuardHit : public Artifact
{
private:
  std::shared_ptr<AnimationComponent> animationComponent; /*!< Animator the effect */
  float w; float h; /*!< Area to appear in */
  bool center; /*!< If true, dink will appear in center of area instead of random */

public:
  /**
   * @brief Load the animation and set position
   */
  GuardHit(std::shared_ptr<Entity> hit, bool center = false);
  ~GuardHit();

  void Init() override;

  /**
   * @brief When the animation ends, it deletes itself
   * @param _elapsed in seconds
   */
  void OnUpdate(double _elapsed) override;

  /**
  * @brief Removes Guard Hit from play
  */
  void OnDelete() override;
};

