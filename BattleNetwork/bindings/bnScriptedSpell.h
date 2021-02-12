#pragma once
#ifdef BN_MOD_SUPPORT

#include <sol/sol.hpp>
#include "../bnSpell.h"
#include "../bnAnimationComponent.h"

using sf::Texture;

/**
 * @class ScriptedSpell
*/
class ScriptedSpell final : public Spell {
public:
  ScriptedSpell(Team _team);
  ~ScriptedSpell();
  
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

  std::function<void(ScriptedSpell*, Battle::Tile&)> spawnCallback;
  std::function<void(ScriptedSpell*, Character*)> attackCallback;
  std::function<bool(Battle::Tile&)> canMoveToCallback;
  std::function<void(ScriptedSpell*)> deleteCallback;
  std::function<void(ScriptedSpell*, double)> updateCallback;
private:
  float height{};
  SpriteProxyNode* shadow{ nullptr };
  AnimationComponent* animComponent{ nullptr };
};
#endif