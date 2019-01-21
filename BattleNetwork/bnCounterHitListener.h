#pragma once
class Character;
class CounterHitPublisher;

class CounterHitListener {
public:
  CounterHitListener() = default;
  ~CounterHitListener() = default;

  CounterHitListener(const CounterHitListener& rhs) = delete;
  CounterHitListener(CounterHitListener&& rhs) = delete;

  virtual void OnCounter(Character& victim, Character& aggressor) = 0;
  void Subscribe(CounterHitPublisher& publisher);
}; 