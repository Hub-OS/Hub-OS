#pragma once

#include <SFML/Audio/SoundBuffer.hpp>
#include <memory>
#include "bnAIState.h"

class Falzar;

class FalzarRoarState : public AIState<Falzar> {
  int animateCount{ 16 };
  int maxAnimateCount{ 16 };
  std::shared_ptr<sf::SoundBuffer> roar;
public:

  /**
   * @brief Sets default cooldown
   */
  FalzarRoarState();

  /**
   * @brief deconstructor
   */
  ~FalzarRoarState();


  void OnEnter(Falzar&);
  void OnUpdate(double _elapsed, Falzar&);
  void OnLeave(Falzar&);
};