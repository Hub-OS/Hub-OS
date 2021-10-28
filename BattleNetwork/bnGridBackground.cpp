#include "bnGridBackground.h"
#include "bnLogger.h"
#include "bnTextureResourceManager.h"
#include "bnGame.h"

#define X_OFFSET 0
#define Y_OFFSET 0

#define COMPONENT_FRAME_COUNT 12
#define COMPONENT_WIDTH 240
#define COMPONENT_HEIGHT 160

GridBackground::GridBackground() : 
  x(0.0f), 
  y(0), 
  progress(0.0f), 
  Background(Textures().LoadFromFile(TexturePaths::NAVI_SELECT_BG), 240, 160)
{
  FillScreen(sf::Vector2u(COMPONENT_WIDTH, COMPONENT_HEIGHT));
}

GridBackground::~GridBackground() {
}

void GridBackground::Update(double _elapsed) {
  progress += 1.0 * _elapsed;
  if (progress >= 1.0) progress = 0;

#ifndef __ANDROID__
  int frame = static_cast<int>(progress * COMPONENT_FRAME_COUNT);
#else
  int frame = 0;
#endif

  TextureOffset(sf::Vector2f((float)(frame*COMPONENT_WIDTH), 0));

}