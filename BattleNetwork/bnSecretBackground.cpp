#include "bnSecretBackground.h"
#include "bnLogger.h"
#include "bnTextureResourceManager.h"
#include "bnDrawWindow.h"

#define PATH std::string("resources/scenes/secret/")

SecretBackground::SecretBackground() : 
  x(0.0f), 
  y(0.0f), 
  Background(Textures().LoadFromFile(PATH + "bg.png"), 240, 160)
{
  FillScreen(sf::Vector2u(240, 160));
}

SecretBackground::~SecretBackground() {
}

void SecretBackground::Update(double _elapsed) {
}