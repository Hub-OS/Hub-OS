#pragma once
#include <functional>
#include <type_traits>
#include "bnAI.h"
#include "bnBossPatternAI.h"
#include "bnCharacter.h"
#include "bnPixelInState.h"
#include "bnStringEncoder.h"
#include "bnNoState.h"
#include "bnMob.h"
#include "bnMobHealthUI.h"

/*! \brief This class handles the semantics of spawning a special character type.
 *
 * \class SpawnPolicy<T>
 *
 * For custom spawning, inherit from this class.
 *
 * Examples: Boss spawner (Alpha has many pieces that need to know about eachother) or
 *           CardSpawner (enemy equiped with a card and CardUIComponent at start)
 */
template<class T>
class SpawnPolicy {
  typedef std::function<void(Character*)> SpawnStateCallback; /*!< Callback functor */

private:
  SpawnStateCallback intro; /*!< Callback after spawning the character */
  SpawnStateCallback ready; /*!< Callback after battle begins */
  T* generate; /*!< The entity spawned */

protected:
  /**
   * @brief Spawns an entity and assigns it to the generate member pointer
   * @param generated
   */
  void Spawn(T* generated) {
    generate = generated;
  }

  /**
   * @brief Assigns the intro callback functor
   * @param intro Callback functor
   *
   * Describes how the entity should behave during the intro animation
   */
  void SetIntroCallback(SpawnStateCallback intro) {
    SpawnPolicy::intro = intro;
  }

  /**
   * @brief Assigns the pre-battle callback functor
   * @param ready Callback functor
   *
   * Describes how the entity should behave after all entities are spawned
   */
  void SetReadyCallback(SpawnStateCallback ready) {
    SpawnPolicy::ready = ready;
  }

public:
  /**
   * @brief Effectively does nothing
   * @param mob reference used by inheriting classes
   */
  SpawnPolicy(Mob& mob) { ; }
  virtual ~SpawnPolicy() { ; }

  /**
   * @brief Returns the spawned entity
   * @return T*
   */
  T* GetSpawned() { return generate; }

  /**
   * @brief Get the intro functor
   * @return intro functor
   */
  SpawnStateCallback& GetIntroCallback() { return intro; }

  /**
   * @brief Get the ready functor
   * @return ready functor
   */
  SpawnStateCallback& GetReadyCallback() { return ready; }
};

/*! \brief Spawn a character with a rank
 *
 *  \cass RankedSpawnPolicty<T>
 *
 * It also registers two specific callbacks in the battle intro:
 * 1) PixelInState and 2) DefaultState
 * For this state, the character will pixelate in.
 * And when the battle begins (cue card select), all character will revert to their DefaultState
 *
 * Ranking affect enemy names and allows the programmer to change other aspects such as appearance
 * e.g. Mettaur, Mettaur2, CanodumbRare1, ProgsmanEX, etc...
*/

template<bool, class T, template <typename> class IntroState>
class RankedSpawnPolicy_t;

template<class T, template <typename> class IntroState>
class RankedSpawnPolicy_t<false, T, IntroState> : public SpawnPolicy<T> {
protected:
    /**
     * @brief Registers pixelate intro and DefaultState ready callbacks
     * @param mob must flag the intro over with FlagNextReady()
     */
    virtual void PrepareCallbacks(Mob &mob) {
      // This retains the current entity type and stores it in a function. We do this to transform the
      // unknown type back later and can call the proper state change
      auto pixelStateInvoker = [&mob](Character* character) {
        auto onFinish = [&mob]() { mob.FlagNextReady(); };

        T* agent = dynamic_cast<T*>(character);

        if (agent) {
          agent->template ChangeState<IntroState<T>>(onFinish);
        }
      };

      auto defaultStateInvoker = [](Character* character) {
        T* agent = dynamic_cast<T*>(character);
        using DefaultState = typename T::DefaultState;

        if (agent) { agent->InvokeDefaultState(); }
      };

      this->SetIntroCallback(pixelStateInvoker);
      this->SetReadyCallback(defaultStateInvoker);
    }

  public:
    RankedSpawnPolicy_t(Mob& mob) : SpawnPolicy<T>(mob) {
      PrepareCallbacks(mob);
    }
};

template<class T, template <typename> class IntroState>
class RankedSpawnPolicy_t<true, T, IntroState> : public SpawnPolicy<T> {
protected:
  /**
   * @brief Registers pixelate intro and DefaultState ready callbacks
   * @param mob must flag the intro over with FlagNextReady()
   */
  virtual void PrepareCallbacks(Mob &mob) {
    // This retains the current entity type and stores it in a function. We do this to transform the
    // unknown type back later and can call the proper state change
    auto pixelStateInvoker = [&mob](Character* character) {
      auto onFinish = [&mob]() { mob.FlagNextReady(); };

      T* agent = dynamic_cast<T*>(character);

      if (agent) {
        agent->template InterruptState<IntroState<T>>(onFinish);
      }
    };

    auto defaultStateInvoker = [](Character* character) {
      T* agent = dynamic_cast<T*>(character);
      using DefaultState = typename T::DefaultState;

      if (agent) { agent->InvokeDefaultState(); }
    };

    this->SetIntroCallback(pixelStateInvoker);
    this->SetReadyCallback(defaultStateInvoker);
  }

public:
  RankedSpawnPolicy_t<true, T, IntroState>(Mob& mob) : SpawnPolicy<T>(mob) {
    PrepareCallbacks(mob);
  }
};

template<typename T, template <typename> class IntroState>
using RankedSpawnPolicy = RankedSpawnPolicy_t< std::is_base_of<BossPatternAI<T>, T>::value, T, IntroState>;


/*! \brief Special implementations of RankedSpawnPolicy
 *
 *  \cass Rank1<T>
 *
 * Automatically constructs an entity with Rank1
 * Adds UI component
*/

template<class T, template <typename> class IntroState=PixelInState>
class Rank1 : public RankedSpawnPolicy<T, IntroState> {
public:
  Rank1(Mob& mob) : RankedSpawnPolicy<T, IntroState>(mob) {
    this->Spawn(new T(T::Rank::_1));
    Component* ui = new MobHealthUI(this->GetSpawned());
    this->GetSpawned()->RegisterComponent(ui);
  }
};

/*! \brief Special implementations of RankedSpawnPolicy
 *
 *  \cass Rank2<T>
 *
 * Automatically constructs an entity with Rank2
 * Adds UI component
*/

template<class T, template <typename> class IntroState = PixelInState>
class Rank2 : public RankedSpawnPolicy<T, IntroState> {
  public:

  Rank2(Mob& mob) : RankedSpawnPolicy<T, IntroState>(mob) {
    this->Spawn(new T(T::Rank::_2));
    this->GetSpawned()->SetName(this->GetSpawned()->GetName() + "2");
    Component* ui = new MobHealthUI(this->GetSpawned());
    this->GetSpawned()->RegisterComponent(ui);
  }
};

/*! \brief Special implementations of RankedSpawnPolicy
 *
 *  \cass Rank3<T>
 *
 * Automatically constructs an entity with Rank3
 * Adds UI component
*/
template<class T, template <typename> class IntroState = PixelInState>
class Rank3 : public RankedSpawnPolicy<T, IntroState> {
  public:

  Rank3(Mob& mob) : RankedSpawnPolicy<T, IntroState>(mob) {
    this->Spawn(new T(T::Rank::_3));
    this->GetSpawned()->SetName(this->GetSpawned()->GetName()+"3");
    Component* ui = new MobHealthUI(this->GetSpawned());
    this->GetSpawned()->RegisterComponent(ui);
  }
};

/*! \brief Special implementations of RankedSpawnPolicy
 *
 *  \cass RankSP<T>
 *
 * Automatically constructs an entity with RankSP
 * Adds UI component
*/

template<class T, template <typename> class IntroState = PixelInState>
class RankSP : public RankedSpawnPolicy<T, IntroState> {
public:

  RankSP(Mob& mob) : RankedSpawnPolicy<T, IntroState>(mob) {
    Spawn(new T(T::Rank::SP));
    this->GetSpawned()->SetName(SP(this->GetSpawned()->GetName()));
    Component* ui = new MobHealthUI(this->GetSpawned());
    this->GetSpawned()->RegisterComponent(ui);
  }
};

/*! \brief Rare1 implementations of RankedSpawnPolicy
 *
 *  \cass RankRare1<T>
 *
 * Automatically constructs an entity with Rank Rare1
 * Adds UI component
*/

template<class T, template <typename> class IntroState = PixelInState>
class RankRare1 : public RankedSpawnPolicy<T, IntroState> {
public:

  RankRare1(Mob& mob) : RankedSpawnPolicy<T, IntroState>(mob) {
    Spawn(new T(T::Rank::Rare1));
    this->GetSpawned()->SetName(this->GetSpawned()->GetName() + " Rare1");
    Component* ui = new MobHealthUI(this->GetSpawned());
    this->GetSpawned()->RegisterComponent(ui);
  }
};

/*! \brief Rare2 implementations of RankedSpawnPolicy
 *
 *  \cass RankRare2<T>
 *
 * Automatically constructs an entity with Rank Rare2
 * Adds UI component
*/

template<class T, template <typename> class IntroState = PixelInState>
class RankRare2 : public RankedSpawnPolicy<T, IntroState> {
public:

  RankRare2(Mob& mob) : RankedSpawnPolicy<T, IntroState>(mob) {
    Spawn(new T(T::Rank::Rare2));
    this->GetSpawned()->SetName(this->GetSpawned()->GetName() + " Rare2");
    Component* ui = new MobHealthUI(this->GetSpawned());
    this->GetSpawned()->RegisterComponent(ui);
  }
};

/*! \brief Special implementations of RankedSpawnPolicy
 *
 *  \cass RankEX<T>
 *
 * Automatically constructs an entity with RankEX
 * Adds UI component
*/

template<class T, template <typename> class IntroState = PixelInState>
class RankEX : public RankedSpawnPolicy<T, IntroState> {
public:

  RankEX(Mob& mob) : RankedSpawnPolicy<T, IntroState>(mob) {
    this->Spawn(new T(T::Rank::EX));
    this->GetSpawned()->SetName(EX(this->GetSpawned()->GetName()));
    Component* ui = new MobHealthUI(this->GetSpawned());
    this->GetSpawned()->RegisterComponent(ui);
  }
};
