#pragma once
#include "bnSpawnPolicy.h"
#include "bnEnemyChipsUI.h"
#include "bnChip.h"

/*
Spawn a character with chips
*/

class ChipSpawnPolicyChipset {
public:
  std::vector<Chip> chips;

  ChipSpawnPolicyChipset() {
    // Test chip
    int random = rand() % 2;

    if (random == 0) {
      chips.push_back(Chip(83, 0, 'K', 0, Element::NONE, "CrckPanel", "Cracks a panel", 2));
    }
    else {
      chips.push_back(Chip(74, 146, '*', 10, Element::NONE, "Recov1", "Heals you", 2));
    }
  }

protected:
  void AddChip(Chip chip) {
    chips.push_back(chip);
  }
};

template<class T, class DefaultState = NoState<T>, class ChipSet = ChipSpawnPolicyChipset>
class ChipsSpawnPolicy : public SpawnPolicy<T> {
private:
  std::vector<Chip> chipset;

protected:
  virtual void PrepareCallbacks(Mob &mob) {
    // This retains the current entity type and stores it in a function. We do this to transform the 
    // unknown type back later and can call the proper state change
    auto pixelStateInvoker = [&mob, this](Character* character) {
      auto onFinish = [&mob]() { mob.FlagNextReady(); };

      T* agent = dynamic_cast<T*>(character);

      if (agent) {
        agent->template ChangeState<PixelInState<T>>(onFinish);
      }
    };

    auto defaultStateInvoker = [](Character* character) {
      T* agent = dynamic_cast<T*>(character);

      if (agent) { agent->template ChangeState<DefaultState>(); }
    };

    this->intro = pixelStateInvoker;
    this->ready = defaultStateInvoker;
  }

public:
  ChipsSpawnPolicy(Mob& mob) : SpawnPolicy<T>(mob) {
    PrepareCallbacks(mob);

    this->generate = new T(T::Rank::_1);

    EnemyChipsUI* ui = new EnemyChipsUI(this->generate);
    ui->LoadChips(ChipSpawnPolicyChipset().chips);
    mob.AddComponent(ui);
  }
};
