#pragma once
#ifdef BN_MOD_SUPPORT

#include <sol/sol.hpp>
#include "dynamic_object.h"
#include "../bnObstacle.h"
#include "../bnAnimationComponent.h"

using sf::Texture;

/**
 * @class ScriptedObstacle
*/
class ScriptedObstacle final : public Obstacle, public dynamic_object {
public:
  ScriptedObstacle(Team _team);
  ~ScriptedObstacle();
  
  void OnUpdate(double _elapsed) override;
  void OnDelete() override;
  bool CanMoveTo(Battle::Tile * next) override;
  void Attack(Character* e) override;
  void OnSpawn(Battle::Tile& spawn) override;
  const float GetHeight() const;
  void SetHeight(const float height);
  void ShowShadow(const bool shadow);

  AnimationComponent& GetAnimationComponent();
  void SetSlideTimeFrames(unsigned frames);

  std::function<void(ScriptedObstacle&, Battle::Tile&)> spawnCallback;
  std::function<void(ScriptedObstacle&, Character&)> attackCallback;
  std::function<bool(Battle::Tile&)> canMoveToCallback;
  std::function<void(ScriptedObstacle&)> deleteCallback;
  std::function<void(ScriptedObstacle&, double)> updateCallback;
private:
  float height{};
  SpriteProxyNode* shadow{ nullptr };
  AnimationComponent* animComponent{ nullptr };
};
#endif