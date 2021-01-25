#include "bnMiscBackground.h"
#include "bnLogger.h"
#include "bnTextureResourceManager.h"
#include "bnGame.h"

#define COMPONENT_WIDTH 32
#define COMPONENT_HEIGHT 32

#define PATH std::string("resources/scenes/misc/")

MiscBackground::MiscBackground()
  : x(0.0f), y(0.0f), Background(Textures().LoadTextureFromFile(PATH + "bg.png"), 240, 180) {
  FillScreen(sf::Vector2u(COMPONENT_WIDTH, COMPONENT_HEIGHT));

  animation = Animation(PATH + "bg.animation");
  animation.Load();
  animation.SetAnimation("BG");
  animation << Animator::Mode::Loop;
}

MiscBackground::~MiscBackground() {
}

void MiscBackground::Update(double _elapsed) {

  animation.Update(_elapsed, dummy);

  y -= static_cast<float>(0.5f * _elapsed);
  x -= static_cast<float>(0.5f *_elapsed);

  if (x < 0) x = 1;
  if (y < 0) y = 1;

  Wrap(sf::Vector2f(x, y));

  // Grab the subrect used in the sprite from animation Update() call
  TextureOffset(dummy.getTextureRect());
}