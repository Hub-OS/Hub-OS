/*! \brief Pixlate intro effect used in the battle intro
 * 
 * This state can be used by any Entity in the engine.
 * It uses constraints to ensure the type passed in Any
 * is a subclass of Entity.
 *
 * This state locks the entity until it comes into focus
 * when pixel shader's factor is equal to 1
 */

#pragma once
#include "bnEntity.h"
#include "bnAIState.h"
#include "bnAudioResourceManager.h"
#include "bnShaderResourceManager.h"
#include <iostream>

typedef std::function<void()> FinishNotifier;

#define MAX_TIME 0.5 // in seconds

template<typename Any>
class PixelInState : public AIState<Any>
{
private:
  SmartShader pixelated; /*!< Shader to pixelate effect */
  float factor{}; /*!< Strength of the pixelate effect. Set to 125 */
  FinishNotifier callback; /*!< Callback when intro effect finished */
public:
  inline static const int PriorityLevel = 2;

  /**
   * \brief sets the finish callback and loads shader
   */
  PixelInState(FinishNotifier onFinish);
  
  /**
   * @brief deconstructor
   */
  ~PixelInState();

  /**
   * @brief Plays intro swoosh sound
   * @param e entity
   */
  void OnEnter(Any& e);
  
  /**
   * @brief When the pixelate effect finishes, invokes the callback
   * @param _elapsed in seconds
   * @param e entity
   */
  void OnUpdate(double _elapsed, Any& e);
  
  /**
   * @brief Revokes the pixelate shader from the entity
   * @param e entity
   */
  void OnLeave(Any& e);
};

#include "bnField.h"
#include "bnLogger.h"

template<typename Any>
PixelInState<Any>::PixelInState(FinishNotifier onFinish) : AIState<Any>() {
  callback = onFinish;
  factor = 125.f;

  pixelated = ResourceHandle().Shaders().GetShader(ShaderType::TEXEL_PIXEL_BLUR);
}

template<typename Any>
PixelInState<Any>::~PixelInState() {
}

template<typename Any>
void PixelInState<Any>::OnEnter(Any& e) {
  // play swoosh
  e.Audio().Play(AudioType::APPEAR);

  sf::Color start = NoopCompositeColor(e.GetColorMode());
  start.a = 0;
  e.setColor(start);
}

template<typename Any>
void PixelInState<Any>::OnUpdate(double _elapsed, Any& e) {
  /* freeze frame */
  e.SetShader(pixelated);

  /* If progress is 1, pop state and move onto original state*/
  factor -= static_cast<float>(_elapsed) * 180.f;

  if (factor <= 0.f) {
    factor = 0.f;

    if (callback) { callback(); callback = nullptr; /* do once */ }

    // TODO: why do we have to set this here??
    e.SetShader(nullptr);
  }

  double range = (180. - factor) / 180.;

  sf::Color start = NoopCompositeColor(e.GetColorMode());
  start.a = static_cast<sf::Uint8>(255 * range);
  e.setColor(start);

  sf::IntRect t = e.getTextureRect();
  sf::Vector2u size = e.getTexture()->getSize();
  pixelated.SetUniform("x", (float)t.left / (float)size.x);
  pixelated.SetUniform("y", (float)t.top / (float)size.y);
  pixelated.SetUniform("w", (float)t.width / (float)size.x);
  pixelated.SetUniform("h", (float)t.height / (float)size.y);
  pixelated.SetUniform("pixel_threshold", (float)(factor/400.f));
}

template<typename Any>
void PixelInState<Any>::OnLeave(Any& e) {}

#undef MAX_TIME