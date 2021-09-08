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

/*! \brief scriptable navi
 *
 * Uses callback functions defined in an external file to configure
 */

class ScriptedPlayer : public Player {
  sol::state& script;
  float height{};

  std::shared_ptr<CardAction> GenerateCardAction(const std::string& functionName);
public:
  friend class PlayerControlledState;
  friend class PlayerIdleState;

  ScriptedPlayer(sol::state& script);

  void Init() override;
  void SetChargePosition(const float x, const float y);
  void SetFullyChargeColor(const sf::Color& color);
  void SetHeight(const float height);
  void SetAnimation(const std::string& path);
  void OnUpdate(double _elapsed) override;
  const float GetHeight() const;
  Animation& GetAnimationObject();
  Battle::Tile* GetCurrentTile() const;

  std::shared_ptr<CardAction> OnExecuteSpecialAction() override final;
  std::shared_ptr<CardAction> OnExecuteBusterAction() override final;
  std::shared_ptr<CardAction> OnExecuteChargedBusterAction() override final;
  std::function<void(std::shared_ptr<ScriptedPlayer>, double)> on_update_func;
};

#endif