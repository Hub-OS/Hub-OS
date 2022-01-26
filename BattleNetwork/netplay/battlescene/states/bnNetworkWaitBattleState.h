#pragma once

#include "../../battlescene/bnNetworkBattleSceneState.h"

struct BattleSceneState;

struct NetworkWaitBattleState : public NetworkBattleSceneState {
  bool ready{ false }; 
  frame_time_t flicker{};
  frame_time_t syncFrame{};

  ~NetworkWaitBattleState();
  void MarkReady();
  bool IsReady() const;
  frame_time_t GetSyncFrame() const;
  void SetSyncFrame(frame_time_t);
  void onStart(const BattleSceneState* next) override;
  void onEnd(const BattleSceneState* last) override;
  void onUpdate(double elapsed) override;
  void onDraw(sf::RenderTexture& surface) override;
};
