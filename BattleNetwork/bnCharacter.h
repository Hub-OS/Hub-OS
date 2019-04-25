#pragma once
#include "bnEntity.h"
#include "bnCounterHitPublisher.h"

#include <string>
#include <queue>

using std::string;

namespace Hit {
  typedef unsigned char Flags;

  const Flags none = 0x00;
  const Flags recoil = 0x01;
  const Flags shake = 0x02;
  const Flags stun = 0x04;
  const Flags pierce = 0x08;
  const Flags flinch = 0x10;
  const Flags breaking = 0x20;
  const Flags impact = 0x40;
  const Flags pushing = 0x80;

  struct Properties {
    int damage;
    Flags flags;
    Element element;
    double secs; // used by both recoil and stun
    Entity* aggressor;
  };

  const Hit::Properties DefaultProperties{ 0, Hit::recoil | Hit::impact, Element::NONE, 3.0, nullptr };

}

class DefenseRule;
class Spell;

class Character : public virtual Entity, public CounterHitPublisher {
  friend class Field;

private:
  int frameDamageTaken;          // accumulation of final damage on frame
  bool frameElementalModifier;   // whether or not the final damage calculated was weak against 
  bool invokeDeletion;
  bool canShareTile;

  Hit::Flags frameHitProps; // accumulation of final hit props on frame

  std::vector<DefenseRule*> defenses;
  
  // Statuses are resolved one property at a time
  // until the entire Flag object is equal to 0x00 None
  // Then we process the next status
  // This continues until all statuses are processed
  std::queue<Hit::Properties> statusQueue;
  
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

  virtual void OnDelete() = 0;
  virtual const bool OnHit(const Hit::Properties props) = 0;
  virtual const float GetHitHeight() const = 0;

  const bool Hit final( const Hit::Properties props = Hit::DefaultProperties);
  virtual void ResolveFrameBattleDamage();
  virtual void Update(float _elapsed);
  virtual bool CanMoveTo(Battle::Tile* next);
  virtual void AddAnimation(string _state, FrameList _frameList, float _duration);
  virtual void SetAnimation(string _state);
  virtual void SetCounterFrame(int frame);
  virtual void OnFrameCallback(int frame, std::function<void()> onEnter, std::function<void()> onLeave = nullptr, bool doOnce = false);
  
  virtual int GetHealth() const;
  const int GetMaxHealth() const;

  void SetHealth(int _health);

  virtual void AdoptTile(Battle::Tile* tile);

  void TryDelete();
  void ToggleCounter(bool on = true);
  void Stun(double maxCooldown);
  bool IsCountered();
  const Rank GetRank() const;

  // Some characters allow others to move ontop of them
  void ShareTileSpace(bool enabled);
  const bool CanShareTileSpace() const;
  void SetPushable(bool enabled);

  // For mob UI
  void SetName(std::string name);
  const std::string GetName() const;

  // For defense rules
  void AddDefenseRule(DefenseRule* rule);
  void RemoveDefenseRule(DefenseRule* rule);

  const bool CheckDefenses(Spell* in);

private:
  int maxHealth;

protected:
  int health;
  bool counterable;
  bool pushable; // used by Hit::pushing
  std::string name;
  double stunCooldown;
  Character::Rank rank;
  sf::Time burnCycle; // how long until a tile burns an entity
  double elapsedBurnTime; // in seconds
};
