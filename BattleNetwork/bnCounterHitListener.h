#pragma once
class Character;
class CounterHitPublisher;

/**
 * @class CounterHitListener
 * @author mav
 * @date 05/05/19
 * @file bnCounterHitListener.h
 * @brief Respond to a counter hit
 * 
 * Some attacks, enemies, or scene logic respond to counter hits from Characters
 */
class CounterHitListener {
public:
  CounterHitListener() = default;
  ~CounterHitListener() = default;

  CounterHitListener(const CounterHitListener& rhs) = delete;
  CounterHitListener(CounterHitListener&& rhs) = delete;

  /**
   * @brief Describe what happens when recieving counter information
   * @param victim who was countered
   * @param aggressor who hit the victim and triggered a counter
   */
  virtual void OnCounter(Character& victim, Character& aggressor) = 0;
  
  /**
   * @brief Subscribe to a potential publisher
   * @param publisher source that a counter event can emit from
   */
  void Subscribe(CounterHitPublisher& publisher);
}; 