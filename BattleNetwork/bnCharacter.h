#pragma once
#include "bnEntity.h"
#include "bnCounterHitPublisher.h"

#include <string>
using std::string;

class Character : public virtual Entity, public CounterHitPublisher {
  friend class Field;

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

  virtual const bool Hit(int damage, Hit::Properties props = Entity::DefaultHitProperties);
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