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
    int random = rand() % 3;

    if (random == 0) {
      chips.push_back(Chip(82, 154, '*', 0, Element::NONE, "AreaGrab", "Defends and reflects", "Press A to bring up a shield that protects you and reflects damage.", 2));
      chips.push_back(Chip(83, 0, 'K', 0, Element::NONE, "CrckPanel", "Cracks a panel", "", 2));
    }
    else if(random == 2) {
      chips.push_back(Chip(75, 147, 'R', 30, Element::NONE, "Recov30", "Recover 30HP", "", 1));
      chips.push_back(Chip(82, 154, '*', 0, Element::NONE, "AreaGrab", "Defends and reflects", "Press A to bring up a shield that protects you and reflects damage.", 2));
    }
    else {
      chips.push_back(Chip(120, 168, '*', 0, Element::NONE, "Barrier", "Nullifies 100HP of damage!", "", 1));
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

    this->SetIntroCallback(pixelStateInvoker);
    this->SetReadyCallback(defaultStateInvoker);
  }

public:
  ChipsSpawnPolicy(Mob& mob) : SpawnPolicy<T>(mob) {
    PrepareCallbacks(mob);

    this->Spawn(new T(T::Rank::_1));

    EnemyChipsUI* ui = new EnemyChipsUI(this->GetSpawned());
    GetSpawned()->RegisterComponent(ui);

    ui->LoadChips(ChipSpawnPolicyChipset().chips);
    mob.DelegateComponent(ui);

    Component* healthui = new MobHealthUI(this->GetSpawned());
    this->GetSpawned()->RegisterComponent(healthui);
  }
};
