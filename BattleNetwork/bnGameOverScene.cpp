#include "bnGameOverScene.h"

GameOverScene::GameOverScene(swoosh::ActivityController& controller) : swoosh::Activity(controller) {
  fadeInCooldown = 500.0f;
}

GameOverScene::~GameOverScene() {

}

void GameOverScene::onStart() {
  AUDIO.Stream("resources/loops/game_over.ogg");
}

void GameOverScene::onUpdate(double elapsed) {
  fadeInCooldown -= elapsed;

  if (fadeInCooldown < 0) {
    fadeInCooldown = 0;
  }
}

void GameOverScene::onDraw(sf::RenderTexture& surface) {
  sf::Vector2f logoPos = (sf::Vector2f)((sf::Vector2i)ENGINE.GetWindow()->getSize() / 2);

  sf::Sprite gameOver;
  gameOver.setTexture(*TEXTURES.GetTexture(TextureType::GAME_OVER));
  gameOver.setScale(2.f, 2.f);
  gameOver.setOrigin(gameOver.getLocalBounds().width / 2, gameOver.getLocalBounds().height / 2);
  gameOver.setPosition(logoPos);

  gameOver.setColor(sf::Color(255, 255, 255, (sf::Uint32)(255 - (255 * (fadeInCooldown / 500.f)))));
  ENGINE.Draw(gameOver);

  // Draw loop
  ENGINE.DrawUnderlay();
  ENGINE.DrawLayers();
  ENGINE.DrawOverlay();

  // Write contents to screen
  //ENGINE.Display();

  // Prepare for next render 
  ENGINE.Clear();
}