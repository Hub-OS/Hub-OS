#pragma once
#include "bnComponent.h"
#include <SFML/Graphics.hpp>
class BattleScene;
class Entity;


class ShakingEffect : public Component {
private:
  BattleSceneBase* bscene;
  Entity* privOwner;
  float shakeDur; /*!< Duration of shake effect */
  double stress; /*!< How much stress to apply to shake */
  bool isShaking;
  float shakeProgress;
  sf::Vector2f startPos;
public:
  ShakingEffect(Entity* owner);
  ~ShakingEffect();
  void OnUpdate(float _elapsed) override;
  void Inject(BattleSceneBase&) override;
};
