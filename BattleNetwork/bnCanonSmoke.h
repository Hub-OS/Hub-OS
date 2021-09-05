#pragma once
#include "bnArtifact.h"
#include "bnField.h"
#include "bnAnimationComponent.h"

/**
 * @class CanonSmoke
 * @author mav
 * @date 05/05/19
 * @brief Animators smoke and deletes after
 */
class CanonSmoke : public Artifact
{
private:
  std::shared_ptr<AnimationComponent> animationComponent;

public:
  CanonSmoke();
  ~CanonSmoke();

  /**
   * @brief animates smoke
   * @param _elapsed in seconds
   */
  void OnUpdate(double _elapsed) override;

  void OnDelete() override;
};

