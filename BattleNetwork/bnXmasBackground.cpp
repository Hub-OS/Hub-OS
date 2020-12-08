#include "bnXmasBackground.h"
#include "bnLogger.h"
#include "bnTextureResourceManager.h"
#include "bnDrawWindow.h"

#define COMPONENT_WIDTH 32
#define COMPONENT_HEIGHT 48

#define PATH std::string("resources/backgrounds/xmas/")

XmasBackground::XmasBackground()
  : x(0.0f), y(0.0f), Background(Textures().LoadTextureFromFile(PATH + "bg.png"), 240, 180) {
  FillScreen(sf::Vector2u(COMPONENT_WIDTH, COMPONENT_HEIGHT));

  animation = Animation(PATH + "bg.animation");
  animation.Load();
  animation.SetAnimation("BG");
  animation << Animator::Mode::Loop;
}

XmasBackground::~XmasBackground() {
}

void XmasBackground::Update(float _elapsed) {

  animation.Update(_elapsed, dummy);

  y -= 0.25f * _elapsed;
  x -= 0.25f *_elapsed;

  if (x < 0) x = 1;
  if (y < 0) y = 1;

  Wrap(sf::Vector2f(x, y));

  // Grab the subrect used in the sprite from animation Update() call
  TextureOffset(dummy.getTextureRect());
}