#include "bnGraveyardBackground.h"
#include "bnLogger.h"
#include "bnTextureResourceManager.h"
#include "bnGame.h"

#define X_OFFSET 0
#define Y_OFFSET 0

#define COMPONENT_FRAME_COUNT 8
#define COMPONENT_WIDTH 128
#define COMPONENT_HEIGHT 32

GraveyardBackground::GraveyardBackground()
  : x(0.0f), y(0.0f), progress(0.0f), Background(TEXTURES.LoadTextureFromFile("resources/scenes/grave/fg.png"), 240, 180) {
  FillScreen(sf::Vector2u(COMPONENT_WIDTH, COMPONENT_HEIGHT));
}

GraveyardBackground::~GraveyardBackground() {
}

void GraveyardBackground::Update(float _elapsed) {
  progress += 0.1f * _elapsed;
  if (progress >= 1.f) progress = 0.0f;

  // crawls
  x += 0.05f *_elapsed;

  if (x > 1) x = 0;
  if (y > 1) y = 0;

  // Progress goes from 0 to 1. Therefore frame goes from 0 to 8.
  int frame = (int)(progress * COMPONENT_FRAME_COUNT);

  // Wrap by x,y movement
  Wrap(sf::Vector2f(x, y));
  
  // Animator by offsetting the texture sample by the frame count
  TextureOffset(sf::Vector2f((float)(frame*COMPONENT_WIDTH), 0));
}