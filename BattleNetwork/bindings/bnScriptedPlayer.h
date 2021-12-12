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
#include "../stx/result.h"
#include "dynamic_object.h"
#include "bnWeakWrapper.h"

/*! \brief scriptable navi
 *
 * Uses callback functions defined in an external file to configure
 */

class ScriptedPlayerFormMeta;

class ScriptedPlayer : public Player, public dynamic_object {
  sol::state& script;
  float height{};

  std::shared_ptr<CardAction> GenerateCardAction(sol::object& function, const std::string& functionName);
  WeakWrapper<ScriptedPlayer> weakWrap;
public:
  friend class PlayerControlledState;
  friend class PlayerIdleState;

  ScriptedPlayer(sol::state& script);
  ~ScriptedPlayer();

  void Init() override;
  void SetChargePosition(const float x, const float y);
  void SetFullyChargeColor(const sf::Color& color);
  void SetHeight(const float height);
  void SetAnimation(const std::string& path);
  void OnUpdate(double _elapsed) override;
  void OnBattleStart() override;
  void OnBattleStop() override;

  ScriptedPlayerFormMeta* CreateForm();

  const float GetHeight() const;
  Animation& GetAnimationObject();
  Battle::Tile* GetCurrentTile() const;

  std::shared_ptr<CardAction> OnExecuteBusterAction() override final;
  std::shared_ptr<CardAction> OnExecuteChargedBusterAction() override final;
  std::shared_ptr<CardAction> OnExecuteSpecialAction() override final;

  sol::object update_func, battle_start_func, battle_end_func, normal_attack_func, charged_attack_func, special_attack_func;
};

#endif