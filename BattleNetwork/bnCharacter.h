#pragma once
#include "bnEntity.h"
#include "bnCounterHitPublisher.h"

#include <string>
using std::string;

namespace Hit {
  typedef char Flags;

  const Flags none = 0x00;
  const Flags recoil = 0x01;
  const Flags shake = 0x02;
  const Flags stun = 0x03;
  const Flags pierce = 0x04;
  const Flags flinch = 0x05;

  struct Properties {
    Flags flags;
    Element element;
    bool impact;
    double secs; // used by both recoil and stun
    Character* aggressor;
  };

  const Hit::Properties DefaultProperties{ Hit::recoil, Element::NONE, true, 3.0, nullptr };

}

class Character : public virtual Entity, public CounterHitPublisher {
  friend class Field;

private:
  int frameDamageTaken;          // accumulation of final damage on frame
  bool frameElementalModifier;   // whether or not the final damage calculated was weak against 
  bool invokeDeletion;

  Hit::Flags frameHitProps; // accumulation of final hit props on frame

public:
  enum class Rank : const int {
    _1,
    _2,
    _3,
    SP,
    EX,
    Rare1,
    Rare2,
    SIZE
  };

  Character(Rank _rank = Rank::_1);
  virtual ~Character();

  virtual const bool Hit(int damage, Hit::Properties props = Hit::DefaultProperties);
  virtual void ResolveFrameBattleDamage();
  virtual void FilterFrameHitsAndApplyGuards(Hit::Flags flags) {} // TODO: make abstract
  virtual const float GetHitHeight() const { return 10.f;  } // TODO: make abstract
  virtual void OnDelete() {} // TODO: make abstract
  virtual void Update(float _elapsed);
  virtual bool CanMoveTo(Battle::Tile* next);
  virtual vector<Drawable*> GetMiscComponents();
  virtual void AddAnimation(string _state, FrameList _frameList, float _duration);
  virtual void SetAnimation(string _state);
  virtual void SetCounterFrame(int frame);
  virtual void OnFrameCallback(int frame, std::function<void()> onEnter, std::function<void()> onLeave = nullptr, bool doOnce = false);
  virtual int* GetAnimOffset();

  virtual int GetHealth() const;
  virtual void SetHealth(int _health);

  void TryDelete();
  void ToggleCounter(bool on = true);
  void Stun(double maxCooldown);
  bool IsCountered();
  const Rank GetRank() const;

  // For mob UI
  void SetName(std::string name);
  const std::string GetName() const;

protected:
  int health;
  bool counterable;
  std::string name;
  double stunCooldown;
  Character::Rank rank;
  sf::Time burnCycle; // how long until a tile burns an entity
  double elapsedBurnTime; // in seconds
};