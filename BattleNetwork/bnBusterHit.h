#pragma once
#include "bnArtifact.h"
#include "bnCharacter.h"
#include "bnAnimationComponent.h"

class Field;

/**
 * @class ChargedBusterHit
 * @author mav
 * @date 05/05/19
 * @brief Animatord hit effect on the enemy and then removes itself 
 */
class BusterHit : public Artifact
{
private:
  AnimationComponent* animationComponent;
  sf::Vector2f offset;
public:
  enum class Type : int {
    PEA,
    CHARGED
  };

  BusterHit(Field* _field, Type type = Type::PEA);
  ~BusterHit();
  void SetOffset(const sf::Vector2f offset);
  void OnUpdate(float _elapsed);
  bool Move(Direction _direction) { return false; }

};

