#pragma once

#include <Swoosh/Timer.h>
#include <SFML/Graphics/Font.hpp>

#include <memory>
#include "../bnBattleSceneState.h"
#include "../../bnTeam.h"
#include "../../bnCardUseListener.h"
#include "../../bnFont.h"

class Character;
class CardAction;
namespace Battle { class Card; }

/* 
    \brief This state governs transitions and combat rules while in TF
*/
struct TimeFreezeBattleState final : public BattleSceneState, CardActionUseListener {
  enum class state : int {
    fadein = 0,
    display_name,
    animate,
    fadeout
  } currState{ state::fadein }, startState{ state::fadein };

  long long lockedTimestamp{ 0 };
  double summonTextLength{ 1.0 }; /*!< How long TFC label should stay on screen */
  double backdropInc{ 1.25 }; //!< alpha increase per frame (max 255)
  bool executeOnce{ false };
  std::string name;
  Team team{ Team::unknown };
  swoosh::Timer summonTimer; /*!< Timer for TFC label to appear at top */
  Character* user{ nullptr }, *stuntDouble{ nullptr };
  CardAction* action{ nullptr };

  TimeFreezeBattleState();
  ~TimeFreezeBattleState();

  void OnCardActionUsed(CardAction* action, uint64_t timestamp) override;
  void CleanupStuntDouble();
  void SkipToAnimateState();
  void onStart(const BattleSceneState* last) override;
  void onEnd(const BattleSceneState* next) override;
  void onUpdate(double elapsed) override;
  void onDraw(sf::RenderTexture& surface) override;
  void ExecuteTimeFreeze();
  const bool FadeOutBackdrop();
  const bool FadeInBackdrop();
  bool IsOver();

  Character* CreateStuntDouble(Character* from);
};
