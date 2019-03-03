#include "bnUndernetBackground.h"
#include "bnLogger.h"
#include "bnTextureResourceManager.h"
#include "bnEngine.h"
#include <Swoosh/Ease.h>

#define X_OFFSET 0
#define Y_OFFSET 0

#define COMPONENT_FRAME_COUNT 2
#define COMPONENT_WIDTH 240
#define COMPONENT_HEIGHT 160
UndernetBackground::UndernetBackground(void)
  : progress(0.0f), Background(*TEXTURES.LoadTextureFromFile("resources/backgrounds/undernet/bg.png"), 240, 180) {
  FillScreen(sf::Vector2u(COMPONENT_WIDTH, COMPONENT_HEIGHT));
  colorIndex = 0;
  
  colors.push_back(sf::Color::Red);
  colors.push_back(sf::Color::Blue);
  colors.push_back(sf::Color::Green);

  colorProgress = 0;
  colorDuration = sf::seconds(5);
}

UndernetBackground::~UndernetBackground(void) {
}

void UndernetBackground::Update(float _elapsed) {
  progress += 10.0f * _elapsed;
  if (progress >= 1.f) progress = 0.0f;

  colorProgress += _elapsed;

  float alpha = swoosh::ease::linear(colorProgress, colorDuration.asSeconds(), 1.0f);

  if (alpha >= 0.75) {
    alpha = swoosh::ease::wideParabola(colorProgress - (colorDuration.asSeconds()*0.75f), 0.30f, 1.0f);

    float r = (colors[colorIndex].r*alpha) + ((1.0f - alpha) * 255);
    float g = (colors[colorIndex].g*alpha) + ((1.0f - alpha) * 255);
    float b = (colors[colorIndex].b*alpha) + ((1.0f - alpha) * 255);

    sf::Color mix = sf::Color((int)r, (int)g, (int)b);

    this->setColor(mix);

    alpha = swoosh::ease::linear(colorProgress - (colorDuration.asSeconds()*0.75f), 0.30f, 1.0f);

    if (alpha == 1.0f) {
      this->setColor(sf::Color::White);
      colorProgress = 0;
      colorIndex = (colorIndex + 1) % (int)colors.size();;
    }
  }

  int frame = (int)(progress * COMPONENT_FRAME_COUNT);

  TextureOffset(sf::Vector2f((float)(frame*COMPONENT_WIDTH), 0));
}
