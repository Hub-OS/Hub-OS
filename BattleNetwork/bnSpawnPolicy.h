#pragma once
#include <functional>
#include "bnAI.h"
#include "bnCharacter.h"
#include "bnPixelInState.h"
#include "bnStringEncoder.h"
#include "bnNoState.h"
#include "bnMob.h"

/*
This class handles the semantics of spawning a special character type.
For custom spawning, inherit from this class.

Examples: Boss spawner (Alpa's core already present) or ChipSpawner (enemy equiped with a chip and ChipUIComponent at start)
*/
template<class T>
class SpawnPolicy {
protected:
  std::function<void(Character*)> intro;
  std::function<void(Character*)> ready;
  T* generate;

  virtual void EnforceConstraints() {
   _DerivedFrom<T, Character>();
   _DerivedFrom<T, AI<T>>();
  }

public:
  SpawnPolicy(Mob& mob) { EnforceConstraints(); }
  virtual ~SpawnPolicy() { ; }
  
  virtual T* GetSpawned() { return generate; }

  virtual std::function<void(Character*)>& GetIntroCallback() { return intro; }
  virtual std::function<void(Character*)>& GetReadyCallback() { return ready; }
};

/*
Spawn a character with a supplied rank 
It also registers two callbacks in the battle intro: 1) PixelInState and 2) DefaultState
For this state, the character will pixelate in. 
And when the battle begins (cue chip select), all character will revert to their DefaultState

Ranking affect enemy names and allows the programmer to change other aspects such as appearance
e.g. Mettaur, Mettaur2, CanodumbRare1, ProgsmanEX, etc...
*/
template<class T, class DefaultState = NoState<T>>
class RankedSpawnPolicy : public SpawnPolicy<T> {
  protected:
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
        
        if (agent) { agent->template ChangeState<DefaultState>(); }
      };

      this->intro = pixelStateInvoker;
      this->ready = defaultStateInvoker;
    }

  public:
    RankedSpawnPolicy(Mob& mob) : SpawnPolicy<T>(mob) {
      PrepareCallbacks(mob);
    }
};

/*
Special implementations of RankedSpawnPolicy
*/

template<class T, class DefaultState = NoState<T>>
class Rank1 : public RankedSpawnPolicy<T, DefaultState> {
public:
  Rank1(Mob& mob) : RankedSpawnPolicy<T, DefaultState>(mob) {
    this->generate = new T(T::Rank::_1);
  }
};

template<class T, class DefaultState = NoState<T>>
class Rank2 : public RankedSpawnPolicy<T, DefaultState> {
  public:

  Rank2(Mob& mob) : RankedSpawnPolicy<T, DefaultState>(mob) {
    this->generate = new T(T::Rank::_2);
  }
};

template<class T, class DefaultState = NoState<T>>
class Rank3 : public RankedSpawnPolicy<T, DefaultState> {
  public:

  Rank3(Mob& mob) : RankedSpawnPolicy<T, DefaultState>(mob) {
    this->generate = new T(T::Rank::_3);
  }
};

template<class T, class DefaultState = NoState<T>>
class RankSP : public RankedSpawnPolicy<T, DefaultState> {
public:

  RankSP(Mob& mob) : RankedSpawnPolicy<T, DefaultState>(mob) {
    this->generate = new T(T::Rank::SP);
    this->generate->SetName(SP(this->generate->GetName()));
  }
};

template<class T, class DefaultState = NoState<T>>
class RankEX : public RankedSpawnPolicy<T, DefaultState> {
public:

  RankEX(Mob& mob) : RankedSpawnPolicy<T, DefaultState>(mob) {
    this->generate = new T(T::Rank::EX);
    this->generate->SetName(EX(this->generate->GetName()));
  }
};
