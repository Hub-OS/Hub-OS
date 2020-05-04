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
  AnimationComponent* animationComponent;

public:
  CanonSmoke(Field* _field);
  ~CanonSmoke();

  /**
   * @brief animates smoke
   * @param _elapsed in seconds
   */
  void OnUpdate(float _elapsed) override;

  void OnDelete() override;
  
  /**
   * @brief Does not move
   * @param _direction ignored
   * @return false
   */
  bool Move(Direction _direction) override;
};

