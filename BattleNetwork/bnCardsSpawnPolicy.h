#pragma once
#include "bnSpawnPolicy.h"
#include "bnEnemyCardsUI.h"
#include "bnCard.h"

/**
 * @class CardSpawnPolicyCardset
 * @author mav
 * @date 05/05/19
 * @brief Spawn a character with hard-coded card values
 */
class CardSpawnPolicyCardset {
public:
  std::vector<Battle::Card> cards;

  CardSpawnPolicyCardset() {

    // NOTE: Need to have some hard-coded cards that can behave just like the Web cards...
    //       Disbabling this for now...
    /*
    // Test card
    int random = rand() % 3;

    if (random == 0) {
      cards.push_back(Battle::Card(82, 154, '*', 0, Element::none, "AreaGrab", "Defends and reflects", "Press A to bring up a shield that protects you and reflects damage.", 2));
      cards.push_back(Battle::Card(83, 0, 'K', 0, Element::none, "CrckPanel", "Cracks a panel", "", 2));
    }
    else if(random == 2) {
      cards.push_back(Battle::Card(75, 147, 'R', 30, Element::none, "Recov30", "Recover 30HP", "", 1));
      cards.push_back(Battle::Card(82, 154, '*', 0, Element::none, "AreaGrab", "Defends and reflects", "Press A to bring up a shield that protects you and reflects damage.", 2));
    }
    else {
      cards.push_back(Battle::Card(120, 168, '*', 0, Element::none, "Barrier", "Nullifies 100HP of damage!", "", 1));
    }*/
  }

protected:
  void AddCard(Battle::Card card) {
    cards.push_back(card);
  }
};


/**
 * @class CardsSpawnPolicy
 * @author mav
 * @date 05/05/19
 * @brief Spawn an entity with cards
 */
template<class T, class DefaultState = NoState<T>, class CardSet = CardSpawnPolicyCardset>
class CardsSpawnPolicy : public SpawnPolicy<T> {
private:
  std::vector<Card> cardset;

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

    SetIntroCallback(pixelStateInvoker);
    SetReadyCallback(defaultStateInvoker);
  }

public:
  CardsSpawnPolicy(Mob& mob) : SpawnPolicy<T>(mob) {
    PrepareCallbacks(mob);

    Spawn(new T(T::Rank::_1));

    EnemyCardsUI* ui = new EnemyCardsUI(GetSpawned());
    GetSpawned()->RegisterComponent(ui);

    ui->LoadCards(Battle::CardSpawnPolicyCardset().cards);
    //mob.DelegateComponent(ui);

    Component* healthui = new MobHealthUI(GetSpawned());
    GetSpawned()->RegisterComponent(healthui);
    //mob.DelegateComponent(healthui);
  }
};
