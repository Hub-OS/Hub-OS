#include "bnUndernetBackground.h"
#include "bnLogger.h"
#include "bnTextureResourceManager.h"
#include "bnGame.h"
#include <Swoosh/Ease.h>

#define X_OFFSET 0
#define Y_OFFSET 0

#define COMPONENT_FRAME_COUNT 2
#define COMPONENT_WIDTH 240
#define COMPONENT_HEIGHT 160
UndernetBackground::UndernetBackground() : 
  progress(0.0f), 
  Background(Textures().LoadTextureFromFile("resources/scenes/undernet/bg.png"), 240, 180)
{
  FillScreen(sf::Vector2u(COMPONENT_WIDTH, COMPONENT_HEIGHT));
  colorIndex = 0;

  // Primary colors to flash
  colors.push_back(sf::Color::Red);
  colors.push_back(sf::Color::Blue);
  colors.push_back(sf::Color::Green);

  colorProgress = 0;
  
  // 5 seconds inbetween color flashes
  colorDuration = sf::seconds(5);
}

UndernetBackground::~UndernetBackground() {
}

void UndernetBackground::Update(double _elapsed) {
  // arbitrary values here: roughly 10 steps per frame
  progress += 10.0 * _elapsed;
  if (progress >= 1) progress = 0.0;

  colorProgress += _elapsed;

  float alpha = swoosh::ease::linear(static_cast<float>(colorProgress), colorDuration.asSeconds(), 1.0f);

  // Only flash colors towards the very end (75%) of the time interval
  if (alpha >= 0.75) {
    // Use a parabola to blend in and out of the color for a more accurate flash
    alpha = swoosh::ease::wideParabola(static_cast<float>(colorProgress) - (colorDuration.asSeconds()*0.75f), 0.30f, 1.0f);

    // Blend the color with the noise texture
    float r = (colors[colorIndex].r*alpha) + ((1.0f - alpha) * 255);
    float g = (colors[colorIndex].g*alpha) + ((1.0f - alpha) * 255);
    float b = (colors[colorIndex].b*alpha) + ((1.0f - alpha) * 255);

    sf::Color mix = sf::Color(static_cast<sf::Uint8>(r), static_cast<sf::Uint8>(g), static_cast<sf::Uint8>(b));

    // Assign the color
    setColor(mix);

    // Use the remaining time to see if we've ended
    alpha = swoosh::ease::linear(static_cast<float>(colorProgress) - (colorDuration.asSeconds()*0.75f), 0.30f, 1.0f);

    if (alpha == 1.0f) {
      // Move onto the next color
      setColor(sf::Color::White);
      colorProgress = 0;
      colorIndex = (colorIndex + 1) % static_cast<int>(colors.size());
    }
  }

  int frame = static_cast<int>(progress * COMPONENT_FRAME_COUNT);

#ifdef __ANDROID__
  frame = 0; // Texture is too large to wrap on mobile and breaks. Make static on android.
#endif

  TextureOffset(sf::Vector2f(static_cast<float>(frame*COMPONENT_WIDTH), 0));
}
