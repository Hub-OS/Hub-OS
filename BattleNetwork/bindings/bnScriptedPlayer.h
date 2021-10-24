#pragma once
#ifdef BN_MOD_SUPPORT

#include <sol/sol.hpp>
#include "../bnPlayer.h"
#include "../bnPlayerState.h"
#include "../bnTextureType.h"
#include "../bnChargeEffectSceneNode.h"
#include "../bnAnimationComponent.h"
#include "../bnAI.h"
#include "../bnPlayerControlledState.h"
#include "../bnPlayerIdleState.h"
#include "../bnPlayerHitState.h"

/*! \brief scriptable navi
 *
 * Uses callback functions defined in an external file to configure
 */

class ScriptedPlayerFormMeta;

class ScriptedPlayer : public Player {
  sol::state& script;
  float height{};

public:
  friend class PlayerControlledState;
  friend class PlayerIdleState;

  ScriptedPlayer(sol::state& script);
  ~ScriptedPlayer();

  void SetChargePosition(const float x, const float y);
  void SetFullyChargeColor(const sf::Color& color);
  void SetHeight(const float height);
  void SetAnimation(const std::string& path);
  void OnUpdate(double _elapsed) override;

  ScriptedPlayerFormMeta* CreateForm();
  void AddForm(ScriptedPlayerFormMeta* meta);

  AnimationComponent::SyncItem& CreateAnimSyncItem(Animation* anim, SpriteProxyNode* node, const std::string& point);
  void RemoveAnimSyncItem(const AnimationComponent::SyncItem& item);

  const float GetHeight() const;
  Animation& GetAnimationObject();
  Battle::Tile* GetCurrentTile() const;

  CardAction* OnExecuteBusterAction() override final;
  CardAction* OnExecuteChargedBusterAction() override final;
  std::function<void(Character&, double)> on_update_func;
  CardAction* OnExecuteSpecialAction() override final;
  std::function<sol::object(Player*)> on_buster_action, on_charged_action, on_special_action;
};

#endif