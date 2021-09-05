#pragma once
#ifdef BN_MOD_SUPPORT

#include <sol/sol.hpp>
#include "dynamic_object.h"
#include "../bnSpell.h"
#include "../bnAnimationComponent.h"

using sf::Texture;

/**
 * @class ScriptedSpell
*/
class ScriptedSpell final : public Spell, public dynamic_object {
public:
  ScriptedSpell(Team _team);
  ~ScriptedSpell();
  
  void OnUpdate(double _elapsed) override;
  void OnDelete() override;
  void OnCollision(const std::shared_ptr<Character> other) override;
  bool CanMoveTo(Battle::Tile * next) override;
  void Attack(std::shared_ptr<Character> e) override;
  void OnSpawn(Battle::Tile& spawn) override;
  const float GetHeight() const;
  void SetHeight(const float height);
  void ShowShadow(const bool shadow);

  void SetAnimation(const std::string& path);
  Animation& GetAnimationObject();
  // duration in seconds
  void ShakeCamera(double power, float duration);
  void NeverFlip(bool enabled);

  std::function<void(std::shared_ptr<ScriptedSpell>, Battle::Tile&)> spawnCallback;
  std::function<void(std::shared_ptr<ScriptedSpell>, std::shared_ptr<Character>)> attackCallback;
  std::function<void(std::shared_ptr<ScriptedSpell>, std::shared_ptr<Character>)> collisionCallback;
  std::function<bool(Battle::Tile&)> canMoveToCallback;
  std::function<void(std::shared_ptr<ScriptedSpell>)> deleteCallback;
  std::function<void(std::shared_ptr<ScriptedSpell>, double)> updateCallback;
private:
  bool flip{true};
  float height{};
  sf::Vector2f scriptedOffset{};
  SpriteProxyNode* shadow{ nullptr };
  std::shared_ptr<AnimationComponent> animComponent{ nullptr };
};
#endif