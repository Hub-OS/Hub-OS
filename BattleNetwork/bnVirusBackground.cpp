#include "bnVirusBackground.h"
#include "bnLogger.h"
#include "bnTextureResourceManager.h"
#include "bnGame.h"

#define X_OFFSET 0
#define Y_OFFSET 0

#define COMPONENT_FRAME_COUNT 8
#define COMPONENT_WIDTH 128
#define COMPONENT_HEIGHT 128

VirusBackground::VirusBackground(void)
  : x(0.0f), 
  y(0), 
  speed(0.0f),
  progress(0.0f), 
  Background(Textures().LoadTextureFromFile("resources/backgrounds/virus/fg.png"), 240, 180) {
  FillScreen(sf::Vector2u(COMPONENT_WIDTH, COMPONENT_HEIGHT));
}

VirusBackground::~VirusBackground() {
}

void VirusBackground::Update(float _elapsed) {
  progress += 0.5f * _elapsed;
  if (progress >= 1.f) progress = 0.0f;

  if (lr == 0) {
    x += 1 * _elapsed * speed;
  }
  else if (ud == 0) {
    y += 1 * _elapsed * speed;
  }

  if (x > 1) x = 0;
  if (y > 1) y = 0;

  int frame = (int)(progress * COMPONENT_FRAME_COUNT);

  Wrap(sf::Vector2f(x, y));
  TextureOffset(sf::Vector2f((float)(frame*COMPONENT_WIDTH), 0));

}

void VirusBackground::ScrollUp()
{
  ud = 0;
  lr = -1;
}

void VirusBackground::ScrollLeft()
{
  ud = -1;
  lr = 0;
}

void VirusBackground::SetScrollSpeed(const float speed)
{
  this->speed = speed;
}
