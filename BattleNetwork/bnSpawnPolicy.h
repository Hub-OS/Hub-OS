#pragma once
#include <functional>
#include "bnAI.h"
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
 *           ChipSpawner (enemy equiped with a chip and ChipUIComponent at start)
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
    this->generate = generated;
  }

  /**
   * @brief Assigns the intro callback functor
   * @param intro Callback functor
   * 
   * Describes how the entity should behave during the intro animation
   */
  void SetIntroCallback(SpawnStateCallback intro) {
    this->intro = intro;
  }

  /**
   * @brief Assigns the pre-battle callback functor
   * @param ready Callback functor
   * 
   * Describes how the entity should behave after all entities are spawned
   */
  void SetReadyCallback(SpawnStateCallback ready) {
    this->ready = ready;
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
  virtual T* GetSpawned() { return generate; }

  /**
   * @brief Get the intro functor
   * @return intro functor
   */
  virtual SpawnStateCallback& GetIntroCallback() { return intro; }
  
  /**
   * @brief Get the ready functor
   * @return ready functor
   */
  virtual SpawnStateCallback& GetReadyCallback() { return ready; }
};

/*! \brief Spawn a character with a rank 
 *  
 *  \cass RankedSpawnPolicty<T>
 * 
 * It also registers two specific callbacks in the battle intro: 
 * 1) PixelInState and 2) DefaultState
 * For this state, the character will pixelate in. 
 * And when the battle begins (cue chip select), all character will revert to their DefaultState
 * 
 * Ranking affect enemy names and allows the programmer to change other aspects such as appearance
 * e.g. Mettaur, Mettaur2, CanodumbRare1, ProgsmanEX, etc...
*/
template<class T>
class RankedSpawnPolicy : public SpawnPolicy<T> {
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
          agent->template ChangeState<PixelInState<T>>(onFinish);
        }
      };

      auto defaultStateInvoker = [](Character* character) { 
        T* agent = dynamic_cast<T*>(character); 
        using DefaultState = typename T::DefaultState;

        if (agent) { agent->template ChangeState<DefaultState>(); }
      };

      this->SetIntroCallback(pixelStateInvoker);
      this->SetReadyCallback(defaultStateInvoker);
    }

  public:
    RankedSpawnPolicy(Mob& mob) : SpawnPolicy<T>(mob) {
      PrepareCallbacks(mob);
    }
};

/*! \brief Special implementations of RankedSpawnPolicy
 * 
 *  \cass Rank1<T>
 * 
 * Automatically constructs an entity with Rank1
 * Adds UI component
*/

template<class T>
class Rank1 : public RankedSpawnPolicy<T> {
public:
  Rank1(Mob& mob) : RankedSpawnPolicy<T>(mob) {
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

template<class T>
class Rank2 : public RankedSpawnPolicy<T> {
  public:

  Rank2(Mob& mob) : RankedSpawnPolicy<T>(mob) {
    this->Spawn(new T(T::Rank::_2));
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
template<class T>
class Rank3 : public RankedSpawnPolicy<T> {
  public:

  Rank3(Mob& mob) : RankedSpawnPolicy<T>(mob) {
    this->Spawn(new T(T::Rank::_3));
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

template<class T>
class RankSP : public RankedSpawnPolicy<T> {
public:

  RankSP(Mob& mob) : RankedSpawnPolicy<T>(mob) {
    this->Spawn(new T(T::Rank::SP));
    this->GetSpawned()->SetName(SP(this->GetSpawned()->GetName()));
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

template<class T>
class RankEX : public RankedSpawnPolicy<T> {
public:

  RankEX(Mob& mob) : RankedSpawnPolicy<T>(mob) {
    this->Spawn(new T(T::Rank::EX));
    this->GetSpawned()->SetName(EX(this->GetSpawned()->GetName()));
    Component* ui = new MobHealthUI(this->GetSpawned());
    this->GetSpawned()->RegisterComponent(ui);
  }
};
