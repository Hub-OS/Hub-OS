#include "bnGameOverScene.h"

GameOverScene::GameOverScene(swoosh::ActivityController& controller) : swoosh::Activity(controller) {
  fadeInCooldown = 5.0f;

  gameOver.setTexture(*TEXTURES.GetTexture(TextureType::GAME_OVER));
  gameOver.setScale(2.f, 2.f);
  gameOver.setOrigin(gameOver.getLocalBounds().width / 2, gameOver.getLocalBounds().height / 2);
}

GameOverScene::~GameOverScene() {

}

void GameOverScene::onStart() {
  fadeInCooldown = 5.0f;
}

void GameOverScene::onResume() {
  AUDIO.StopStream();
  AUDIO.Stream("resources/loops/game_over.ogg");
}

void GameOverScene::onUpdate(double elapsed) {
  fadeInCooldown -= (float)elapsed;

  if (fadeInCooldown < 0) {
    fadeInCooldown = 0;
  }
}

void GameOverScene::onDraw(sf::RenderTexture& surface) {
  ENGINE.SetRenderSurface(surface);

  sf::Vector2f logoPos = (sf::Vector2f)((sf::Vector2i)ENGINE.GetWindow()->getSize() / 2);
  gameOver.setPosition(logoPos);
  gameOver.setColor(sf::Color(255, 255, 255, (sf::Uint32)(255 * (1.0f-(fadeInCooldown / 5.f)))));
  ENGINE.Draw(gameOver);
}