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

  void Init() override;
  void OnUpdate(double _elapsed) override;
  void OnDelete() override;
  bool CanMoveTo(Battle::Tile * next) override;
  void OnCollision(const std::shared_ptr<Character> other) override;
  void Attack(std::shared_ptr<Character> e) override;
  void OnSpawn(Battle::Tile& spawn) override;
  const float GetHeight() const;
  void SetHeight(const float height);
  void ShowShadow(const bool shadow);

  void SetAnimation(const std::string& path);
  Animation& GetAnimationObject();
  Battle::Tile* GetCurrentTile() const;

  // duration in seconds
  void ShakeCamera(double power, float duration);

  void NeverFlip(bool enabled);

  std::function<void(std::shared_ptr<ScriptedObstacle>, Battle::Tile&)> spawnCallback;
  std::function<void(std::shared_ptr<ScriptedObstacle>, std::shared_ptr<Character>)> attackCallback;
  std::function<void(std::shared_ptr<ScriptedObstacle>, std::shared_ptr<Character>)> collisionCallback;
  std::function<bool(Battle::Tile&)> canMoveToCallback;
  std::function<void(std::shared_ptr<ScriptedObstacle>)> deleteCallback;
  std::function<void(std::shared_ptr<ScriptedObstacle>, double)> updateCallback;
private:
  bool flip{ true };
  sf::Vector2f scriptedOffset{};
  float height{};
  SpriteProxyNode* shadow{ nullptr };
  std::shared_ptr<AnimationComponent> animComponent{ nullptr };
  std::shared_ptr<DefenseRule> obstacleBody{ nullptr };
};
#endif