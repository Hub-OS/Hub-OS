#pragma once
#include "bnSpawnPolicy.h"

/*
Spawn Alpha
*/

class Alpha;

template<typename T>
class GlowInState;

class AlphaBossSpawnPolicy : public SpawnPolicy<Alpha> {
protected:
  virtual void PrepareCallbacks(Mob &mob) {
    auto introStateInvoker = [&mob](Character* character) {
      auto onFinish = [&mob]() { mob.FlagNextReady(); };

      // Todo, each piece of alpha appears one at a time, glowing
      // Field* field = mob.GetField();

      // AlphaCore* core = new AlphaCore(); // register with mob how?
      //
      // AlphaHead* head = new AlphaHead();
      // field.OwnEntity(head, 5, 2);
      // 
      // AlphaLeftClaw* left = new AlphaLeftClaw();
      // field.OwnEntity(left, 4, 1);
      // 
      // AlphaRightClaw* right = new AlphaRightClaw();
      // field.OwnEntity(right, 5, 3);
      //
      // core->SetHead(head);
      // core->SetLeftClaw(left);
      // core->SetRightClaw(right);
      //
      // core->SetState<GlowInState>(onFinish); // To use states it must inherit AI<>
      // head->SetState<GlowInState>(onFinish);
      // left->SetState<GlowInState>(onFinish);
      // right->SetState<GlowInState>(onFinish);
    };

    auto defaultStateInvoker = [](Character* character) {
        // AlphaCore* core = dynamic_cast<AlphaCore*>(character);
        // core->DefaultState ??
    };

    this->SetIntroCallback(introStateInvoker);
    this->SetReadyCallback(defaultStateInvoker);
  }

public:
  AlphaBossSpawnPolicy(Mob& mob) : SpawnPolicy<Alpha>(mob) {
    PrepareCallbacks(mob);
  }
};
