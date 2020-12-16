#include "bnRobotBackground.h"
#include "bnLogger.h"
#include "bnTextureResourceManager.h"
#include "bnGame.h"

#define COMPONENT_WIDTH 64
#define COMPONENT_HEIGHT 64

#define PATH std::string("resources/scenes/robot/")

RobotBackground::RobotBackground()
  : x(0.0f), y(0.0f), Background(Textures().LoadTextureFromFile(PATH + "bg.png"), 240, 180) {
  FillScreen(sf::Vector2u(COMPONENT_WIDTH, COMPONENT_HEIGHT));

  animation = Animation(PATH + "bg.animation");
  animation.Load();
  animation.SetAnimation("BG");
  animation << Animator::Mode::Loop;
}

RobotBackground::~RobotBackground() {
}

void RobotBackground::Update(double _elapsed) {

  animation.Update(_elapsed, dummy);

  y -= 0.5f * _elapsed;

  if (y < 0) y = 1;

  Wrap(sf::Vector2f(0, y));

  // Grab the subrect used in the sprite from animation Update() call
  TextureOffset(dummy.getTextureRect());
}